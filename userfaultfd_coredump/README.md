# userfaultfd on-demand in-process core dump (experiment)

A standalone, Linux-only experiment that writes an ELF core dump of the current
process *on demand* while the process keeps running, using **userfaultfd
write-protect (WP) mode** to get a point-in-time consistent snapshot **without
`fork()`**.

This is deliberately dependency-free (libc + Linux headers only) so it can be
iterated on in isolation before any integration into the main codebase.

## Why userfaultfd (and the honest caveat)

userfaultfd is a page-fault *delivery* mechanism, not a memory-*reading* one. It
does not dump memory for you — `process_vm_readv` does that here. Its one real
job in this design is the **consistency layer**: without `fork()`'s copy-on-write
snapshot, WP-mode is what lets a background dumper stream a coherent "as of
time T" image while mutator threads keep writing. A write that races the dumper
trips a WP fault; the fault handler preserves that page's original contents
*before* letting the write land.

If you did not need a torn-free snapshot, you would not need userfaultfd at all —
reading `/proc/self/maps` + `process_vm_readv` directly would do. WP-mode is the
price of "no fork, still consistent".

## How it works

1. **Arm()** — create the `userfaultfd`, do the `UFFDIO_API` handshake, require
   `UFFD_FEATURE_PAGEFAULT_FLAG_WP` (Linux ≥ 5.7), pre-spawn the monitor thread,
   and pre-allocate every buffer (claim bitmap, scratch pages). Nothing on the
   dump path allocates afterwards.
2. **Trigger()** —
   - parse `/proc/self/maps`, WP-register large mutable private regions,
   - lay out the ELF file (header, program headers, per-region data offsets),
   - WP-protect all registered regions (this instant is time **T**),
   - the **dumper** streams every page to the core via `process_vm_readv`,
   - a mutator write that beats the dumper to a page trips a WP fault; the
     **monitor** preserves the original page, then clears WP to release the writer,
   - a per-page atomic **claim bitmap** guarantees each page is written exactly
     once, by whichever of {dumper, monitor} reaches it first.
3. **Shutdown()** — stop the monitor, unregister regions, close the uffd.

## Key invariants (the parts that bite)

- The monitor thread must **never fault on a region it services** — its stack,
  buffers, and the claim bitmap live in unregistered memory, pre-touched at Arm().
- **No allocation on the fault path** — a stalled writer may hold the malloc lock.
- Writes are stalled only long enough to preserve one page.

## Build & run (Linux)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/uffd_coredump_demo /tmp/ondemand.core 256 4 2
#                           ^out              ^MiB ^threads ^seconds
```

The demo spins mutator threads hammering a large buffer, triggers a dump
mid-write, and checks the output is `ET_CORE`.

## Verify the dump

```sh
eu-readelf -n /tmp/ondemand.core      # notes + program headers
readelf -l /tmp/ondemand.core         # PT_LOAD / PT_NOTE segments
gdb /path/to/uffd_coredump_demo /tmp/ondemand.core   # open as a core
```

## Verified on WSL2 (kernel 5.15, g++ 13.3)

Built with `-Wall -Wextra -Wpedantic` (no warnings) and exercised end to end:

- **Deadlock-free under write pressure.** The demo triggers a dump while 4
  mutator threads hammer a 64 MiB buffer; the dump completes (~200 ms) and the
  process exits cleanly. Earlier revisions deadlocked — see "Root-cause notes".
- **Structurally valid core.** `readelf -h/-l` reports `Type: CORE`, 1 `PT_NOTE`
  + 41 `PT_LOAD` segments with correct vaddr/size/flags; gdb opens it as
  `elf64-x86-64` and maps every segment. (gdb warns "Couldn't find
  general-purpose registers" — expected, the `PT_NOTE` is intentionally empty.)
- **Correct contents.** `pattern_demo` fills a buffer with `0xAB`, dumps, and
  gdb reads the bytes back out of the core at the buffer's address as `0xAB` —
  the dump captures real memory, not zeros.

Reproduce (requires root to toggle the sysctl on a hardened kernel):

```sh
sudo bash run_as_root.sh         # build+run demo under write pressure
sudo bash inspect_core.sh        # readelf + gdb structural check
sudo bash verify_contents.sh     # pattern round-trip through the core
sudo bash finalize.sh            # restore sysctl, strict-warning rebuild
```

## Root-cause notes (two bugs found and fixed during bring-up)

1. **Non-resident pages are invisible to WP.** `probe_fault.c` demonstrates that
   WP-protecting a lazily-allocated (non-resident) page delivers *no* userfault
   on first write — the kernel zero-fills it as an ordinary minor fault. So the
   dumper now protects only *resident* pages (via `/proc/self/pagemap`); a
   non-resident page's value at T is zero and needs no protection.
2. **The resolver must not fault on its own memory.** The monitor/dumper write
   the claim bitmap and scratch pages while resolving faults. When those lived
   on the glibc heap — itself a WP-protected region — the monitor self-faulted
   and the whole process wedged (every thread parked in `handle_userfault`;
   captured with `diagnose_wchan.sh`). Fixed by moving the bitmap and scratch
   into dedicated `mmap` regions explicitly excluded from WP registration, and
   running the dump on a dedicated thread so the caller's stack is never
   protected.

## Known limitations / next steps

- **Notes are minimal.** The `PT_NOTE` segment is emitted empty. A production
  version needs `NT_PRPSINFO`, `NT_PRSTATUS` (per thread), `NT_FILE`, and
  `NT_AUXV` for gdb to show threads/registers. Only the triggering thread's
  state is meaningful anyway, since other threads keep running.
- **Requires `vm.unprivileged_userfaultfd=1`** or `CAP_SYS_PTRACE` on hardened
  kernels; `Arm()` returns `PermissionDenied` otherwise.
- **Assumes 4 KiB pages** and `x86_64` (`EM_X86_64` in the ELF header). Both are
  easy to generalize.
- **Hugetlb / shared mappings** are not WP-registered; they are read directly
  (torn-page risk) or skipped.

