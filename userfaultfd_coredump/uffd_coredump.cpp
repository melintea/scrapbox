// uffd_coredump.cpp -- see uffd_coredump.h for the design overview.
//
// Standalone experiment: libc + Linux headers only.

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
#include <sys/eventfd.h>
#include <poll.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <condition_variable>
#include <mutex>
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

// Query /proc/self/pagemap for the resident pages of [start, end). For each
// page we set out_present[i] = true iff the page is backed by physical memory
// right now (bit 63 of the pagemap entry). pagemap_fd must be an open fd on
// /proc/self/pagemap. Returns false on a read error (caller treats all as
// non-resident, i.e. zero, which is the safe/correct fallback).
bool query_residency(int pagemap_fd, std::uintptr_t start, std::size_t n_pages,
                     bool* out_present)
{
    constexpr std::uint64_t kPresentBit = std::uint64_t{1} << 63;
    const off_t off = static_cast<off_t>((start / kPageSize) * sizeof(std::uint64_t));

    // Read in bounded chunks into a fixed stack buffer -- no heap allocation on
    // the dump path.
    constexpr std::size_t kChunkPages = 512;
    std::uint64_t entries[kChunkPages];

    std::size_t done = 0;
    while (done < n_pages) {
        const std::size_t batch = std::min(kChunkPages, n_pages - done);
        const off_t batch_off = off + static_cast<off_t>(done * sizeof(std::uint64_t));
        std::size_t got = 0;
        while (got < batch * sizeof(std::uint64_t)) {
            const ssize_t r = ::pread(pagemap_fd,
                                      reinterpret_cast<char*>(entries) + got,
                                      batch * sizeof(std::uint64_t) - got,
                                      batch_off + static_cast<off_t>(got));
            if (r < 0) {
                if (errno == EINTR) { continue; }
                return false;
            }
            if (r == 0) { return false; }
            got += static_cast<std::size_t>(r);
        }
        for (std::size_t i = 0; i < batch; ++i) {
            out_present[done + i] = (entries[i] & kPresentBit) != 0;
        }
        done += batch;
    }
    return true;
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
    int pagemap_fd = -1;           // open fd on /proc/self/pagemap
    bool armed = false;

    // Monitor thread (services WP faults) and dumper thread (streams pages).
    // The dump runs entirely off the caller's thread so the caller's stack is
    // never WP-protected and cannot self-fault.
    int shutdown_evt = -1;         // eventfd: ask the monitor/dumper to exit
    int run_evt      = -1;         // eventfd: ask the dumper to run one job
    int done_evt     = -1;         // eventfd: dumper signals the waiting caller
    std::thread monitor;
    std::thread dumper;

    // Per-dump state (valid only while a Trigger() is in flight).
    std::atomic<bool> dumping{false};
    std::atomic<bool> dump_done{false};
    std::atomic<int>  dump_result{0};   // Status encoded as int for the caller
    int out_fd = -1;
    std::string pending_path;           // set by Trigger() before kicking dumper

    // Regions captured at Trigger() time, in address order.
    std::vector<Region> regions;

    // Per-page claim bitmap: one atomic word per 64 pages. The thread that
    // flips a page's bit from 0->1 owns writing that page to the core.
    //
    // CRITICAL: this and the scratch pages are mmap'd into DEDICATED regions
    // that parse_maps_and_register() excludes from WP registration. The monitor
    // and dumper write to them while handling faults; if they lived on the
    // glibc heap (a WP-protected region) those writes would self-fault and
    // deadlock the resolver. All engine-owned mmap ranges are recorded in
    // engine_ranges and skipped during registration.
    std::atomic<std::uint64_t>* claim_words = nullptr;  // mmap'd at Arm()
    std::size_t claim_words_count = 0;
    std::size_t claim_words_bytes = 0;
    std::size_t total_pages = 0;

    // One scratch page per worker so the dump path never allocates.
    void* dumper_scratch  = nullptr;
    void* monitor_scratch = nullptr;

    // Address ranges of engine-owned mmap allocations (claim bitmap, scratch),
    // excluded from WP registration so the dump path never self-faults.
    struct AddrRange { std::uintptr_t start; std::uintptr_t end; };
    AddrRange engine_ranges[4] = {};
    std::size_t engine_range_count = 0;

    void add_engine_range(void* p, std::size_t len)
    {
        if (engine_range_count < 4 && p != nullptr && p != MAP_FAILED) {
            const auto s = reinterpret_cast<std::uintptr_t>(p);
            engine_ranges[engine_range_count++] = {s, s + len};
        }
    }

    bool overlaps_engine(std::uintptr_t start, std::uintptr_t end) const
    {
        for (std::size_t i = 0; i < engine_range_count; ++i) {
            if (start < engine_ranges[i].end && engine_ranges[i].start < end) {
                return true;
            }
        }
        return false;
    }

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
    bool parse_maps_and_register();     // runs on the dumper thread
    void monitor_loop();                // runs on the monitor thread
    void dumper_loop();                 // runs on the dumper thread
    Status run_one_dump();              // the actual dump, on the dumper thread
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

        // Never WP-register a thread stack: the dumper/monitor threads (and the
        // blocked caller) write their stacks constantly and would self-fault.
        // The main stack is "[stack]"; glibc thread stacks are anonymous but we
        // keep them unprotected by excluding the known engine-thread stacks via
        // the stack-guard heuristic below is not reliable, so we simply never
        // protect any region tagged as a stack and accept torn reads elsewhere.
        const bool is_stack = (r.path == "[stack]");

        // Never protect the engine's own mmap allocations (claim bitmap,
        // scratch): the monitor/dumper write them while resolving faults.
        const bool is_engine = overlaps_engine(r.start, r.end);

        const bool anonymous_private = (perms[3] == 'p') && r.path.empty();
        const bool big_enough = r.size() >= opts.min_wp_region_bytes;
        const bool want_wp =
            writable && anonymous_private && big_enough && !is_stack && !is_engine;

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

    // Pre-allocate the claim bitmap for the worst case in a DEDICATED mmap
    // region (not the heap), so the monitor/dumper can write it while handling
    // faults without self-faulting. One atomic word per 64 pages.
    d.claim_words_count = (d.opts.max_tracked_pages + 63) / 64;
    d.claim_words_bytes = ((d.claim_words_count * sizeof(std::atomic<std::uint64_t>))
                           + kPageSize - 1) & ~(kPageSize - 1);
    void* bitmap = ::mmap(nullptr, d.claim_words_bytes, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (bitmap == MAP_FAILED) {
        d.set_err("mmap claim bitmap", errno);
        ::close(d.uffd); d.uffd = -1;
        return Status::SystemError;
    }
    std::memset(bitmap, 0, d.claim_words_bytes);  // resident + zeroed now
    d.claim_words = static_cast<std::atomic<std::uint64_t>*>(bitmap);
    d.add_engine_range(bitmap, d.claim_words_bytes);

    // Pre-allocate per-worker scratch pages (page-aligned, never WP-registered).
    d.dumper_scratch  = ::mmap(nullptr, kPageSize, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    d.monitor_scratch = ::mmap(nullptr, kPageSize, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (d.dumper_scratch == MAP_FAILED || d.monitor_scratch == MAP_FAILED) {
        d.set_err("mmap scratch pages", errno);
        ::munmap(bitmap, d.claim_words_bytes); d.claim_words = nullptr;
        ::close(d.uffd); d.uffd = -1;
        return Status::SystemError;
    }
    // Touch them now so the monitor never faults populating its own scratch.
    std::memset(d.dumper_scratch, 0, kPageSize);
    std::memset(d.monitor_scratch, 0, kPageSize);
    d.add_engine_range(d.dumper_scratch, kPageSize);
    d.add_engine_range(d.monitor_scratch, kPageSize);

    d.shutdown_evt = ::eventfd(0, EFD_CLOEXEC);
    d.run_evt      = ::eventfd(0, EFD_CLOEXEC);
    d.done_evt     = ::eventfd(0, EFD_CLOEXEC);
    if (d.shutdown_evt < 0 || d.run_evt < 0 || d.done_evt < 0) {
        d.set_err("eventfd", errno);
        if (d.shutdown_evt >= 0) { ::close(d.shutdown_evt); d.shutdown_evt = -1; }
        if (d.run_evt >= 0)      { ::close(d.run_evt);      d.run_evt = -1; }
        if (d.done_evt >= 0)     { ::close(d.done_evt);     d.done_evt = -1; }
        ::close(d.uffd); d.uffd = -1;
        return Status::SystemError;
    }

    d.pagemap_fd = ::open("/proc/self/pagemap", O_RDONLY | O_CLOEXEC);
    if (d.pagemap_fd < 0) {
        d.set_err("open /proc/self/pagemap", errno);
        ::close(d.shutdown_evt); d.shutdown_evt = -1;
        ::close(d.run_evt);      d.run_evt = -1;
        ::close(d.done_evt);     d.done_evt = -1;
        ::close(d.uffd);         d.uffd = -1;
        return Status::SystemError;
    }

    d.monitor = std::thread([&d] { d.monitor_loop(); });
    d.dumper  = std::thread([&d] { d.dumper_loop(); });
    d.armed = true;
    d.last_error.clear();
    return Status::Ok;
}

// Dumper thread: waits for a run_evt kick, performs one dump, publishes the
// result, and loops. Running the dump here (not on the caller's thread) keeps
// the caller's stack out of WP-protected memory so it cannot self-fault.
void Dumper::Impl::dumper_loop()
{
    pollfd fds[2];
    fds[0].fd = run_evt;      fds[0].events = POLLIN;
    fds[1].fd = shutdown_evt; fds[1].events = POLLIN;

    for (;;) {
        const int pr = ::poll(fds, 2, -1);
        if (pr < 0) {
            if (errno == EINTR) { continue; }
            break;
        }
        if (fds[1].revents & POLLIN) { break; }        // shutdown
        if ((fds[0].revents & POLLIN) == 0) { continue; }

        std::uint64_t ticks = 0;
        (void)!::read(run_evt, &ticks, sizeof ticks);  // drain the kick

        const Status s = run_one_dump();
        dump_result.store(static_cast<int>(s), std::memory_order_relaxed);
        dump_done.store(true, std::memory_order_release);
        // Wake a caller blocked in Trigger().
        const std::uint64_t one = 1;
        (void)!::write(done_evt, &one, sizeof one);
    }
}

// The actual dump, entirely on the dumper thread.
Status Dumper::Impl::run_one_dump()
{
    Status result = Status::Ok;

    out_fd = ::open(pending_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (out_fd < 0) {
        set_err("open core file", errno);
        return Status::WriteError;
    }

    for (std::size_t i = 0; i < claim_words_count; ++i) {
        claim_words[i].store(0, std::memory_order_relaxed);
    }

    // 1) Enumerate regions and WP-register the large mutable private ones.
    if (!parse_maps_and_register()) {
        set_err(last_error.empty() ? "no dumpable regions" : last_error.c_str(), 0);
        ::close(out_fd); out_fd = -1;
        return Status::SystemError;
    }

    // 2) Lay out the ELF file: header + N program headers (1 NOTE + 1 per
    //    region), then assign each region its data file offset.
    const std::size_t n_phdr = regions.size() + 1;  // +1 for PT_NOTE
    const std::uint64_t ehdr_size = sizeof(Elf64_Ehdr);
    const std::uint64_t phdr_size = sizeof(Elf64_Phdr) * n_phdr;
    std::uint64_t cursor = ehdr_size + phdr_size;
    const std::uint64_t note_offset = cursor;
    const std::uint64_t note_size = 0;  // empty PT_NOTE (see README limitations)
    cursor += note_size;
    cursor = (cursor + kPageSize - 1) & ~(static_cast<std::uint64_t>(kPageSize) - 1);
    for (Region& r : regions) {
        r.file_offset = cursor;
        cursor += r.size();
    }

    // 3) Protect ONLY resident pages -- this instant is time T. Non-resident
    //    pages read as zero at T and are left unprotected (a later write to
    //    one is invisible to WP anyway; its T-value was zero regardless).
    {
        bool present[512];
        for (const Region& r : regions) {
            if (!r.wp) { continue; }
            const std::size_t npages = r.pages();
            std::size_t p = 0;
            while (p < npages) {
                const std::size_t batch = std::min<std::size_t>(512, npages - p);
                if (!query_residency(pagemap_fd, r.start + p * kPageSize, batch, present)) {
                    // On a pagemap read error, conservatively protect the whole
                    // batch (treat as resident) so we never miss a live page.
                    for (std::size_t i = 0; i < batch; ++i) { present[i] = true; }
                }
                // Protect each maximal run of resident pages in one ioctl.
                std::size_t i = 0;
                while (i < batch) {
                    if (!present[i]) { ++i; continue; }
                    std::size_t j = i;
                    while (j < batch && present[j]) { ++j; }
                    set_wp(r.start + (p + i) * kPageSize, (j - i) * kPageSize);
                    i = j;
                }
                p += batch;
            }
        }
    }

    // 4) Write ELF header + program headers.
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
        if (!write_all_at(out_fd, &eh, sizeof eh, 0)) { result = Status::WriteError; }

        // One phdr buffer, written in bounded chunks -- the phdr vector is
        // engine state (unregistered heap), safe to touch during the dump.
        std::vector<Elf64_Phdr> phdrs;
        phdrs.reserve(n_phdr);
        Elf64_Phdr note{};
        note.p_type = PT_NOTE; note.p_offset = note_offset; note.p_filesz = note_size;
        phdrs.push_back(note);
        for (const Region& r : regions) {
            Elf64_Phdr ph{};
            ph.p_type   = PT_LOAD;
            ph.p_offset = r.file_offset;
            ph.p_vaddr  = r.start;
            ph.p_filesz = r.size();
            ph.p_memsz  = r.size();
            ph.p_flags  = r.flags;
            ph.p_align  = kPageSize;
            phdrs.push_back(ph);
        }
        if (!write_all_at(out_fd, phdrs.data(), phdrs.size() * sizeof(Elf64_Phdr), ehdr_size)) {
            result = Status::WriteError;
        }
    }

    // 5) Stream every page the monitor hasn't already preserved.
    if (result == Status::Ok) {
        for (const Region& r : regions) {
            for (std::size_t p = 0; p < r.pages(); ++p) {
                const std::size_t global_page = r.page_base + p;
                if (!claim_page(global_page)) { continue; }  // monitor got it first
                const std::uintptr_t page_addr = r.start + p * kPageSize;
                if (!dump_one_page(r, page_addr, dumper_scratch)) {
                    result = Status::WriteError;
                    break;
                }
                if (r.wp) { clear_wp(page_addr, kPageSize); }
            }
            if (result != Status::Ok) { break; }
        }
    }

    // 6) Tear down this dump's protection and state.
    unregister_all();
    ::fsync(out_fd);
    ::close(out_fd);
    out_fd = -1;

    if (result != Status::Ok && last_error.empty()) {
        set_err("core write failed", 0);
    } else if (result == Status::Ok) {
        last_error.clear();
    }
    return result;
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

    // Hand the job to the dumper thread. The caller's thread does NOTHING that
    // touches a WP-protected page from here until the dump completes -- it only
    // blocks reading an eventfd, so it can never self-fault.
    d.pending_path = out_path;
    d.dump_done.store(false, std::memory_order_release);
    {
        const std::uint64_t one = 1;
        (void)!::write(d.run_evt, &one, sizeof one);
    }

    // Wait for completion on the done eventfd (no heap/condvar touches).
    std::uint64_t ticks = 0;
    while (!d.dump_done.load(std::memory_order_acquire)) {
        const ssize_t n = ::read(d.done_evt, &ticks, sizeof ticks);
        if (n < 0 && errno == EINTR) { continue; }
        if (n < 0) { break; }
    }

    const Status result = static_cast<Status>(d.dump_result.load(std::memory_order_relaxed));
    d.dumping.store(false, std::memory_order_release);
    return result;
}

Status Dumper::Shutdown()
{
    Impl& d = *m_impl;
    if (!d.armed) { return Status::NotArmed; }

    // Ask both worker threads to exit. The eventfd is never drained by the
    // loops (they only break on POLLIN), so a single post stays readable and
    // both threads observe it.
    if (d.shutdown_evt >= 0) {
        const std::uint64_t one = 1;
        (void)!::write(d.shutdown_evt, &one, sizeof one);
    }
    if (d.dumper.joinable())  { d.dumper.join(); }
    if (d.monitor.joinable()) { d.monitor.join(); }

    d.unregister_all();

    if (d.dumper_scratch && d.dumper_scratch != MAP_FAILED) {
        ::munmap(d.dumper_scratch, kPageSize);
    }
    if (d.monitor_scratch && d.monitor_scratch != MAP_FAILED) {
        ::munmap(d.monitor_scratch, kPageSize);
    }
    d.dumper_scratch = d.monitor_scratch = nullptr;

    if (d.claim_words != nullptr) {
        ::munmap(d.claim_words, d.claim_words_bytes);
        d.claim_words = nullptr;
        d.claim_words_count = 0;
    }

    if (d.shutdown_evt >= 0) { ::close(d.shutdown_evt); d.shutdown_evt = -1; }
    if (d.run_evt >= 0)      { ::close(d.run_evt);      d.run_evt = -1; }
    if (d.done_evt >= 0)     { ::close(d.done_evt);     d.done_evt = -1; }
    if (d.pagemap_fd >= 0)   { ::close(d.pagemap_fd);   d.pagemap_fd = -1; }
    if (d.uffd >= 0)         { ::close(d.uffd);         d.uffd = -1; }

    d.regions.clear();
    d.engine_range_count = 0;
    d.armed = false;
    return Status::Ok;
}

const std::string& Dumper::last_error() const { return m_impl->last_error; }

}  // namespace uffd_coredump

#endif  // __linux__
