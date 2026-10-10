// uffd_coredump.cpp -- see uffd_coredump.h for the design overview.
//
// Standalone experiment: libc + Linux headers only, no ION dependencies.

#include "uffd_coredump.h"

#include <atomic>
#include <cstring>
#include <string>

namespace uffd_coredump {

const char* to_string(Status s)
{
    switch (s) {
        case Status::Ok:               return "Ok";
        case Status::Unsupported:      return "Unsupported";
        case Status::NotArmed:         return "NotArmed";
        case Status::AlreadyArmed:     return "AlreadyArmed";
        case Status::AlreadyRunning:   return "AlreadyRunning";
        case Status::PermissionDenied: return "PermissionDenied";
        case Status::SystemError:      return "SystemError";
        case Status::WriteError:       return "WriteError";
    }
    return "?";
}

}  // namespace uffd_coredump

// ===========================================================================
// Non-Linux fallback: everything is a no-op that reports Unsupported.
// ===========================================================================
#if !defined(__linux__)

namespace uffd_coredump {

struct Dumper::Impl { std::string last_error = "userfaultfd core dump is Linux-only"; };

Dumper::Dumper() : m_impl(new Impl) {}
Dumper::~Dumper() { delete m_impl; }
Status Dumper::Arm(const Options&)            { return Status::Unsupported; }
Status Dumper::Trigger(const std::string&)    { return Status::Unsupported; }
Status Dumper::Shutdown()                     { return Status::Unsupported; }
const std::string& Dumper::last_error() const { return m_impl->last_error; }
bool Dumper::PlatformSupported()              { return false; }

}  // namespace uffd_coredump

#else  // __linux__
// ===========================================================================
// Linux implementation.
// ===========================================================================

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include <elf.h>
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <thread>
#include <vector>

namespace uffd_coredump {
namespace {

constexpr std::size_t kPageSize = 4096;  // verified against sysconf at Arm()

// A readable region of the address space, parsed from /proc/self/maps.
struct Region {
    std::uintptr_t start = 0;
    std::uintptr_t end   = 0;
    std::uint32_t  flags = 0;        // PF_R / PF_W / PF_X (ELF program header flags)
    bool           wp    = false;    // true => WP-registered (mutable snapshot)
    std::size_t    page_base = 0;    // index of first page in the claim bitmap
    std::uint64_t  file_offset = 0;  // where this region's data lands in the core
    std::string    path;             // backing file ("" for anonymous)

    std::size_t size() const { return static_cast<std::size_t>(end - start); }
    std::size_t pages() const { return size() / kPageSize; }
};

int sys_userfaultfd(int flags)
{
    return static_cast<int>(::syscall(SYS_userfaultfd, flags));
}

// Write exactly `len` bytes at file offset `off`, retrying short writes/EINTR.
// Async-signal-safe-ish: no allocation, only pwrite.
bool write_all_at(int fd, const void* buf, std::size_t len, off_t off)
{
    const char* p = static_cast<const char*>(buf);
    while (len > 0) {
        const ssize_t n = ::pwrite(fd, p, len, off);
        if (n < 0) {
            if (errno == EINTR) { continue; }
            return false;
        }
        if (n == 0) { return false; }
        p   += n;
        off += n;
        len -= static_cast<std::size_t>(n);
    }
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------
struct Dumper::Impl {
    Options opts;
    std::string last_error;

    int uffd = -1;
    bool armed = false;

    // Shutdown signalling for the monitor thread.
    int shutdown_evt = -1;         // eventfd, written to ask the monitor to exit
    std::thread monitor;

    // Per-dump state (valid only while a Trigger() is in flight).
    std::atomic<bool> dumping{false};
    int out_fd = -1;

    // Regions captured at Trigger() time, in address order.
    std::vector<Region> regions;

    // Per-page claim bitmap: one atomic word per 64 pages. The thread that
    // flips a page's bit from 0->1 owns writing that page to the core.
    std::vector<std::atomic<std::uint64_t>> claim_words;  // pre-sized at Arm()
    std::size_t total_pages = 0;

    // One scratch page per worker so the dump path never allocates.
    void* dumper_scratch  = nullptr;
    void* monitor_scratch = nullptr;

    // --- helpers ---------------------------------------------------------
    void set_err(const char* what, int e)
    {
        last_error = what;
        if (e != 0) {
            last_error += ": ";
            last_error += std::strerror(e);
        }
    }

    // Atomically claim `global_page`. Returns true if THIS caller won the claim.
    bool claim_page(std::size_t global_page)
    {
        const std::size_t word = global_page >> 6;
        const std::uint64_t bit = std::uint64_t{1} << (global_page & 63u);
        const std::uint64_t prev = claim_words[word].fetch_or(bit, std::memory_order_acq_rel);
        return (prev & bit) == 0;
    }

    // Write one already-claimed page's contents to the core file.
    bool dump_one_page(const Region& r, std::uintptr_t page_addr, void* scratch)
    {
        // Read the page from our own address space via process_vm_readv so a
        // not-yet-populated page reads as zero rather than faulting us.
        iovec local{scratch, kPageSize};
        iovec remote{reinterpret_cast<void*>(page_addr), kPageSize};
        const ssize_t n = ::process_vm_readv(::getpid(), &local, 1, &remote, 1, 0);
        if (n < 0) {
            // Unreadable page: emit zeros so file offsets stay consistent.
            std::memset(scratch, 0, kPageSize);
        } else if (static_cast<std::size_t>(n) < kPageSize) {
            std::memset(static_cast<char*>(scratch) + n, 0, kPageSize - static_cast<std::size_t>(n));
        }

        const std::size_t page_in_region = (page_addr - r.start) / kPageSize;
        const off_t off = static_cast<off_t>(r.file_offset + page_in_region * kPageSize);
        return write_all_at(out_fd, scratch, kPageSize, off);
    }

    bool handshake();
    bool parse_maps_and_register();
    void write_elf_headers();  // header + program headers (fixed-size prelude)
    void monitor_loop();
    void clear_wp(std::uintptr_t start, std::size_t len);
    void set_wp(std::uintptr_t start, std::size_t len);
    void unregister_all();
};

// ---------------------------------------------------------------------------
// UFFDIO_API handshake and feature check.
// ---------------------------------------------------------------------------
bool Dumper::Impl::handshake()
{
    uffd = sys_userfaultfd(O_CLOEXEC | O_NONBLOCK);
    if (uffd < 0) {
        set_err("userfaultfd()", errno);
        return false;
    }

    uffdio_api api{};
    api.api = UFFD_API;
#ifdef UFFD_FEATURE_PAGEFAULT_FLAG_WP
    api.features = UFFD_FEATURE_PAGEFAULT_FLAG_WP;
#else
    api.features = 0;
#endif
    if (::ioctl(uffd, UFFDIO_API, &api) < 0) {
        set_err("UFFDIO_API", errno);
        return false;
    }
#ifdef UFFD_FEATURE_PAGEFAULT_FLAG_WP
    if ((api.features & UFFD_FEATURE_PAGEFAULT_FLAG_WP) == 0) {
        set_err("kernel lacks UFFD_FEATURE_PAGEFAULT_FLAG_WP (need Linux >= 5.7)", 0);
        return false;
    }
#else
    set_err("built against headers without UFFD WP support", 0);
    return false;
#endif
    return true;
}

void Dumper::Impl::set_wp(std::uintptr_t start, std::size_t len)
{
#ifdef UFFDIO_WRITEPROTECT
    uffdio_writeprotect wp{};
    wp.range.start = start;
    wp.range.len   = len;
    wp.mode        = UFFDIO_WRITEPROTECT_MODE_WP;
    ::ioctl(uffd, UFFDIO_WRITEPROTECT, &wp);  // best-effort; failures are per-page tolerated
#else
    (void)start; (void)len;
#endif
}

void Dumper::Impl::clear_wp(std::uintptr_t start, std::size_t len)
{
#ifdef UFFDIO_WRITEPROTECT
    uffdio_writeprotect wp{};
    wp.range.start = start;
    wp.range.len   = len;
    wp.mode        = 0;  // clear WP
    ::ioctl(uffd, UFFDIO_WRITEPROTECT, &wp);
#else
    (void)start; (void)len;
#endif
}

// ---------------------------------------------------------------------------
// Parse /proc/self/maps, build the region list, WP-register the mutable ones.
// ---------------------------------------------------------------------------
bool Dumper::Impl::parse_maps_and_register()
{
    regions.clear();
    total_pages = 0;

    // Read maps with raw read() -- getline/ifstream would allocate on the heap
    // while we are about to protect the heap. A fixed stack buffer is enough
    // for a single line.
    const int mfd = ::open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
    if (mfd < 0) {
        set_err("open /proc/self/maps", errno);
        return false;
    }

    std::string contents;
    {
        char buf[8192];
        for (;;) {
            const ssize_t n = ::read(mfd, buf, sizeof buf);
            if (n < 0) {
                if (errno == EINTR) { continue; }
                set_err("read /proc/self/maps", errno);
                ::close(mfd);
                return false;
            }
            if (n == 0) { break; }
            contents.append(buf, static_cast<std::size_t>(n));
        }
    }
    ::close(mfd);

    std::uint64_t next_offset = 0;  // running file offset, filled after prelude sizing

    std::size_t line_start = 0;
    while (line_start < contents.size()) {
        std::size_t line_end = contents.find('\n', line_start);
        if (line_end == std::string::npos) { line_end = contents.size(); }
        const std::string line = contents.substr(line_start, line_end - line_start);
        line_start = line_end + 1;
        if (line.empty()) { continue; }

        std::uintptr_t start = 0, end = 0;
        char perms[8] = {0};
        char path[4096] = {0};
        // Format: start-end perms offset dev inode path
        const int matched = std::sscanf(
            line.c_str(), "%" SCNxPTR "-%" SCNxPTR " %7s %*x %*x:%*x %*u %4095[^\n]",
            &start, &end, perms, path);
        if (matched < 3) { continue; }
        if (end <= start) { continue; }

        const bool readable = perms[0] == 'r';
        const bool writable = perms[1] == 'w';
        if (!readable) { continue; }  // can't read it => can't dump it

        // Skip special regions that must never be dumped or protected.
        if (std::strcmp(path, "[vvar]") == 0 ||
            std::strcmp(path, "[vdso]") == 0 ||
            std::strcmp(path, "[vsyscall]") == 0) {
            continue;
        }

        Region r;
        r.start = start;
        r.end   = end;
        r.flags = (readable ? PF_R : 0u) | (writable ? PF_W : 0u) |
                  (perms[2] == 'x' ? PF_X : 0u);
        r.path  = (matched >= 4) ? path : "";

        const bool anonymous_private = (perms[3] == 'p') && r.path.empty();
        const bool big_enough = r.size() >= opts.min_wp_region_bytes;
        const bool want_wp = writable && anonymous_private && big_enough;

        if (!writable && !opts.include_readonly_regions) {
            continue;  // caller only wants the mutable snapshot
        }

        if (total_pages + r.pages() > opts.max_tracked_pages) {
            // Bitmap would overflow -- stop registering further regions rather
            // than exceed the Arm()-time allocation. Already-added regions dump.
            break;
        }

        r.page_base = total_pages;
        total_pages += r.pages();

        if (want_wp) {
#ifdef UFFDIO_REGISTER_MODE_WP
            uffdio_register reg{};
            reg.range.start = r.start;
            reg.range.len   = r.size();
            reg.mode        = UFFDIO_REGISTER_MODE_WP;
            if (::ioctl(uffd, UFFDIO_REGISTER, &reg) == 0) {
                r.wp = true;
            }
            // If registration fails (e.g. region type unsupported for WP), we
            // keep the region but read it directly (torn-page risk accepted).
#endif
        }

        regions.push_back(std::move(r));
    }

    (void)next_offset;
    return !regions.empty();
}

// ---------------------------------------------------------------------------
// Monitor thread: resolve WP faults by preserving the original page first.
// ---------------------------------------------------------------------------
void Dumper::Impl::monitor_loop()
{
    pollfd fds[2];
    fds[0].fd = uffd;         fds[0].events = POLLIN;
    fds[1].fd = shutdown_evt; fds[1].events = POLLIN;

    for (;;) {
        const int pr = ::poll(fds, 2, -1);
        if (pr < 0) {
            if (errno == EINTR) { continue; }
            break;
        }

        if (fds[1].revents & POLLIN) {
            break;  // shutdown requested
        }
        if ((fds[0].revents & POLLIN) == 0) { continue; }

        uffd_msg msg;
        const ssize_t n = ::read(uffd, &msg, sizeof msg);
        if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) { continue; }
            break;
        }
        if (n != sizeof msg) { continue; }
        if (msg.event != UFFD_EVENT_PAGEFAULT) { continue; }

        const std::uintptr_t fault_addr =
            static_cast<std::uintptr_t>(msg.arg.pagefault.address) & ~(kPageSize - 1);

        // Only WP faults carry original-content preservation semantics.
        const bool is_wp =
#ifdef UFFD_PAGEFAULT_FLAG_WP
            (msg.arg.pagefault.flags & UFFD_PAGEFAULT_FLAG_WP) != 0;
#else
            false;
#endif

        // Find the owning region (linear scan: region count is small).
        const Region* owner = nullptr;
        for (const Region& r : regions) {
            if (fault_addr >= r.start && fault_addr < r.end) { owner = &r; break; }
        }

        if (owner != nullptr && dumping.load(std::memory_order_acquire)) {
            const std::size_t global_page =
                owner->page_base + (fault_addr - owner->start) / kPageSize;
            if (claim_page(global_page)) {
                // We won the race against the dumper: preserve the ORIGINAL
                // page contents (the write has not landed yet) before release.
                dump_one_page(*owner, fault_addr, monitor_scratch);
            }
        }

        // Release the writer: clear WP so the stalled write proceeds.
        if (is_wp) {
            clear_wp(fault_addr, kPageSize);
        }
    }
}

void Dumper::Impl::unregister_all()
{
    for (const Region& r : regions) {
        if (!r.wp) { continue; }
#ifdef UFFDIO_UNREGISTER
        uffdio_range range{};
        range.start = r.start;
        range.len   = r.size();
        ::ioctl(uffd, UFFDIO_UNREGISTER, &range);
#endif
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
Dumper::Dumper() : m_impl(new Impl) {}

Dumper::~Dumper()
{
    if (m_impl->armed) { Shutdown(); }
    delete m_impl;
}

bool Dumper::PlatformSupported() { return true; }

Status Dumper::Arm(const Options& opts)
{
    Impl& d = *m_impl;
    if (d.armed) { d.set_err("already armed", 0); return Status::AlreadyArmed; }

    d.opts = opts;

    if (static_cast<std::size_t>(::sysconf(_SC_PAGESIZE)) != kPageSize) {
        d.set_err("unexpected page size (module assumes 4096)", 0);
        return Status::Unsupported;
    }

    if (!d.handshake()) {
        if (d.uffd >= 0) { ::close(d.uffd); d.uffd = -1; }
        return (errno == EPERM) ? Status::PermissionDenied : Status::Unsupported;
    }

    // Pre-allocate the claim bitmap for the worst case, so Trigger() never
    // allocates it. One atomic word per 64 pages.
    const std::size_t words = (d.opts.max_tracked_pages + 63) / 64;
    d.claim_words = std::vector<std::atomic<std::uint64_t>>(words);

    // Pre-allocate per-worker scratch pages (page-aligned, never WP-registered).
    d.dumper_scratch  = ::mmap(nullptr, kPageSize, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    d.monitor_scratch = ::mmap(nullptr, kPageSize, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (d.dumper_scratch == MAP_FAILED || d.monitor_scratch == MAP_FAILED) {
        d.set_err("mmap scratch pages", errno);
        ::close(d.uffd); d.uffd = -1;
        return Status::SystemError;
    }
    // Touch them now so the monitor never faults populating its own scratch.
    std::memset(d.dumper_scratch, 0, kPageSize);
    std::memset(d.monitor_scratch, 0, kPageSize);

    d.shutdown_evt = ::eventfd(0, EFD_CLOEXEC);
    if (d.shutdown_evt < 0) {
        d.set_err("eventfd", errno);
        ::close(d.uffd); d.uffd = -1;
        return Status::SystemError;
    }

    d.monitor = std::thread([&d] { d.monitor_loop(); });
    d.armed = true;
    d.last_error.clear();
    return Status::Ok;
}

Status Dumper::Trigger(const std::string& out_path)
{
    Impl& d = *m_impl;
    if (!d.armed) { d.set_err("not armed", 0); return Status::NotArmed; }

    bool expected = false;
    if (!d.dumping.compare_exchange_strong(expected, true)) {
        d.set_err("a dump is already running", 0);
        return Status::AlreadyRunning;
    }

    Status result = Status::Ok;

    d.out_fd = ::open(out_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (d.out_fd < 0) {
        d.set_err("open core file", errno);
        d.dumping.store(false, std::memory_order_release);
        return Status::WriteError;
    }

    // Clear any stale claims from a prior dump.
    for (auto& w : d.claim_words) { w.store(0, std::memory_order_relaxed); }

    // 1) Enumerate regions and WP-register + WP-protect the mutable ones.
    if (!d.parse_maps_and_register()) {
        d.set_err(d.last_error.empty() ? "no dumpable regions" : d.last_error.c_str(), 0);
        ::close(d.out_fd); d.out_fd = -1;
        d.dumping.store(false, std::memory_order_release);
        return Status::SystemError;
    }

    // 2) Lay out the ELF file: header + N program headers (1 NOTE + 1 per
    //    region), then assign each region its data file offset, then protect.
    const std::size_t n_load = d.regions.size();
    const std::size_t n_phdr = n_load + 1;  // +1 for PT_NOTE

    std::vector<std::uint8_t> note_buf;
    // Minimal NT_PRPSINFO-free note set; a real build would add PRSTATUS/PRPSINFO.
    // We still emit an (empty) PT_NOTE so tools see a well-formed core.

    const std::uint64_t ehdr_size = sizeof(Elf64_Ehdr);
    const std::uint64_t phdr_size = sizeof(Elf64_Phdr) * n_phdr;
    std::uint64_t cursor = ehdr_size + phdr_size;

    const std::uint64_t note_offset = cursor;
    const std::uint64_t note_size = note_buf.size();
    cursor += note_size;

    // Page-align the first PT_LOAD's data for cleanliness.
    cursor = (cursor + kPageSize - 1) & ~(static_cast<std::uint64_t>(kPageSize) - 1);
    for (Region& r : d.regions) {
        r.file_offset = cursor;
        cursor += r.size();
    }

    // Protect every WP-registered region now -- this is time T.
    for (const Region& r : d.regions) {
        if (r.wp) { d.set_wp(r.start, r.size()); }
    }

    // 3) Write ELF header + program headers.
    {
        Elf64_Ehdr eh{};
        std::memcpy(eh.e_ident, ELFMAG, SELFMAG);
        eh.e_ident[EI_CLASS]   = ELFCLASS64;
        eh.e_ident[EI_DATA]    = ELFDATA2LSB;
        eh.e_ident[EI_VERSION] = EV_CURRENT;
        eh.e_type    = ET_CORE;
        eh.e_machine = EM_X86_64;
        eh.e_version = EV_CURRENT;
        eh.e_phoff   = ehdr_size;
        eh.e_ehsize  = sizeof(Elf64_Ehdr);
        eh.e_phentsize = sizeof(Elf64_Phdr);
        eh.e_phnum   = static_cast<Elf64_Half>(n_phdr);
        if (!write_all_at(d.out_fd, &eh, sizeof eh, 0)) { result = Status::WriteError; }

        std::vector<Elf64_Phdr> phdrs;
        phdrs.reserve(n_phdr);

        Elf64_Phdr note{};
        note.p_type   = PT_NOTE;
        note.p_offset = note_offset;
        note.p_filesz = note_size;
        phdrs.push_back(note);

        for (const Region& r : d.regions) {
            Elf64_Phdr ph{};
            ph.p_type   = PT_LOAD;
            ph.p_offset = r.file_offset;
            ph.p_vaddr  = r.start;
            ph.p_paddr  = 0;
            ph.p_filesz = r.size();
            ph.p_memsz  = r.size();
            ph.p_flags  = r.flags;
            ph.p_align  = kPageSize;
            phdrs.push_back(ph);
        }
        if (!write_all_at(d.out_fd, phdrs.data(),
                          phdrs.size() * sizeof(Elf64_Phdr), ehdr_size)) {
            result = Status::WriteError;
        }
        if (note_size > 0 &&
            !write_all_at(d.out_fd, note_buf.data(), note_size,
                          static_cast<off_t>(note_offset))) {
            result = Status::WriteError;
        }
    }

    // 4) Dumper pass: stream every page the monitor hasn't already claimed.
    if (result == Status::Ok) {
        for (const Region& r : d.regions) {
            for (std::size_t p = 0; p < r.pages(); ++p) {
                const std::size_t global_page = r.page_base + p;
                if (!d.claim_page(global_page)) {
                    continue;  // monitor already preserved this page
                }
                const std::uintptr_t page_addr = r.start + p * kPageSize;
                if (!d.dump_one_page(r, page_addr, d.dumper_scratch)) {
                    result = Status::WriteError;
                    break;
                }
                // Already-dumped mutable page: drop WP so writers stop faulting.
                if (r.wp) { d.clear_wp(page_addr, kPageSize); }
            }
            if (result != Status::Ok) { break; }
        }
    }

    // 5) Tear down this dump's protection and state.
    d.unregister_all();
    ::fsync(d.out_fd);
    ::close(d.out_fd);
    d.out_fd = -1;
    d.dumping.store(false, std::memory_order_release);

    if (result != Status::Ok && d.last_error.empty()) {
        d.set_err("core write failed", 0);
    } else if (result == Status::Ok) {
        d.last_error.clear();
    }
    return result;
}

Status Dumper::Shutdown()
{
    Impl& d = *m_impl;
    if (!d.armed) { return Status::NotArmed; }

    // Ask the monitor to exit and join it.
    if (d.shutdown_evt >= 0) {
        const std::uint64_t one = 1;
        (void)!::write(d.shutdown_evt, &one, sizeof one);
    }
    if (d.monitor.joinable()) { d.monitor.join(); }

    d.unregister_all();

    if (d.dumper_scratch && d.dumper_scratch != MAP_FAILED) {
        ::munmap(d.dumper_scratch, kPageSize);
    }
    if (d.monitor_scratch && d.monitor_scratch != MAP_FAILED) {
        ::munmap(d.monitor_scratch, kPageSize);
    }
    d.dumper_scratch = d.monitor_scratch = nullptr;

    if (d.shutdown_evt >= 0) { ::close(d.shutdown_evt); d.shutdown_evt = -1; }
    if (d.uffd >= 0)         { ::close(d.uffd);         d.uffd = -1; }

    d.regions.clear();
    d.claim_words.clear();
    d.armed = false;
    return Status::Ok;
}

const std::string& Dumper::last_error() const { return m_impl->last_error; }

}  // namespace uffd_coredump

#endif  // __linux__
