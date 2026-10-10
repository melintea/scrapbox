// uffd_coredump.h
//
// On-demand, in-process core dump for Linux, using userfaultfd write-protect
// (WP) mode to obtain a point-in-time consistent snapshot WITHOUT fork().
//
// Linux-only (write-protect faults require kernel >= 5.7). On any other
// platform the API compiles to a no-op that reports "unsupported".
//
// Design summary
// --------------
//  * At Arm() time we create a userfaultfd, perform the UFFDIO_API handshake,
//    and pre-spawn a monitor thread plus pre-allocate every buffer used on the
//    dump path. Nothing on the dump path allocates afterwards.
//  * On Trigger() we enumerate /proc/self/maps, WP-register and WP-protect the
//    large mutable private regions, then stream every page to an ELF core file
//    on the dumper thread. Mutator writes that race the dumper trip a WP fault;
//    the monitor thread preserves the original page BEFORE releasing the writer.
//  * A per-page atomic claim bitmap guarantees each page is written exactly
//    once, by whichever of {dumper, monitor} reaches it first.
//
// This is a standalone experiment module: libc + Linux headers only.

#ifndef UFFD_COREDUMP_H
#define UFFD_COREDUMP_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace uffd_coredump {

enum class Status {
    Ok = 0,
    Unsupported,       // not Linux, or kernel lacks WP-mode userfaultfd
    NotArmed,          // Trigger()/Shutdown() called before Arm()
    AlreadyArmed,      // Arm() called twice
    AlreadyRunning,    // Trigger() called while a dump is in progress
    PermissionDenied,  // userfaultfd() blocked (vm.unprivileged_userfaultfd=0)
    SystemError,       // a syscall failed; see errno-style detail in message
    WriteError,        // failed to write the core file
};

const char* to_string(Status s);

struct Options {
    // Minimum region size (bytes) to WP-register. Smaller mutable regions are
    // read directly without snapshot protection (negligible torn-page risk).
    std::size_t min_wp_region_bytes = 64 * 1024;

    // Upper bound on total pages we will track in the claim bitmap. Caps the
    // pre-allocated bitmap so Arm() never surprises us at Trigger() time.
    std::size_t max_tracked_pages = 8u * 1024 * 1024;  // 32 GiB at 4 KiB pages

    // If true, include non-WP-registerable readable regions (text, rodata,
    // read-only file maps) in the dump, read directly. If false, only the
    // WP-registered mutable regions are dumped (smaller, faster).
    bool include_readonly_regions = true;

    // Append the triggering thread's register state as NT_PRSTATUS. Other
    // threads keep running and are not sampled (memory image, not full state).
    bool include_self_thread_regs = true;
};

// Opaque engine. One instance per process is the expected usage.
class Dumper {
public:
    Dumper();
    ~Dumper();

    Dumper(const Dumper&) = delete;
    Dumper& operator=(const Dumper&) = delete;

    // Create the uffd, handshake, pre-spawn the monitor, pre-allocate buffers.
    // Safe to call once; idempotent failure returns AlreadyArmed on re-arm.
    Status Arm(const Options& opts = Options{});

    // Produce a core dump at `out_path` reflecting memory as of the call.
    // Blocking: returns when the full image has been written. May be called
    // from a signal-safe trampoline only indirectly (set a flag, call from a
    // normal thread) -- Trigger() itself is NOT async-signal-safe.
    Status Trigger(const std::string& out_path);

    // Stop the monitor thread, unregister regions, close the uffd.
    Status Shutdown();

    // Human-readable detail about the last failure (empty on success).
    const std::string& last_error() const;

    static bool PlatformSupported();

private:
    struct Impl;
    Impl* m_impl;
};

}  // namespace uffd_coredump

#endif  // UFFD_COREDUMP_H
