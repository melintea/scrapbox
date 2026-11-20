/* probe_fault.c -- minimal, self-contained probe to determine how the kernel
 * delivers faults for a WP-registered-but-not-yet-resident anonymous region.
 *
 * This isolates the root cause of the demo deadlock without any of the
 * module's machinery. It:
 *   1. mmaps an anonymous region,
 *   2. registers it WP-only with userfaultfd,
 *   3. WP-protects it,
 *   4. from a worker thread, writes one byte to a page,
 *   5. the monitor prints the fault flags it receives.
 *
 * If the fault carries UFFD_PAGEFAULT_FLAG_WP, WP delivery works and the
 * module's bug is purely "dumper runs on protected memory". If instead the
 * worker blocks with NO fault delivered (or a non-WP fault), the lazy-
 * population interaction is also in play.
 *
 * Build: gcc -O2 -pthread probe_fault.c -o probe_fault
 * Run as root with: echo 1 > /proc/sys/vm/unprivileged_userfaultfd (or CAP)
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#define PAGE 4096
#define NPAGES 16

static int uffd = -1;
static char *region;

static void *monitor(void *arg) {
    (void)arg;
    struct pollfd pfd = {.fd = uffd, .events = POLLIN};
    for (;;) {
        int pr = poll(&pfd, 1, 2000);
        if (pr == 0) { printf("monitor: poll timeout (no fault delivered)\n"); return NULL; }
        if (pr < 0) { if (errno == EINTR) continue; perror("poll"); return NULL; }
        struct uffd_msg msg;
        ssize_t n = read(uffd, &msg, sizeof msg);
        if (n != sizeof msg) { continue; }
        if (msg.event != UFFD_EVENT_PAGEFAULT) continue;
        unsigned long long fl = msg.arg.pagefault.flags;
        printf("monitor: PAGEFAULT addr=%p flags=0x%llx WP=%d WRITE=%d\n",
               (void *)msg.arg.pagefault.address, fl,
               (fl & UFFD_PAGEFAULT_FLAG_WP) ? 1 : 0,
               (fl & UFFD_PAGEFAULT_FLAG_WRITE) ? 1 : 0);
        /* resolve by clearing WP on that page so the writer proceeds */
        struct uffdio_writeprotect wp;
        memset(&wp, 0, sizeof wp);
        wp.range.start = msg.arg.pagefault.address & ~(uintptr_t)(PAGE - 1);
        wp.range.len = PAGE;
        wp.mode = 0;
        if (ioctl(uffd, UFFDIO_WRITEPROTECT, &wp) < 0) perror("resolve UFFDIO_WRITEPROTECT");
        printf("monitor: resolved; exiting\n");
        return NULL;
    }
}

static void *writer(void *arg) {
    (void)arg;
    printf("writer: writing to page 3 ...\n");
    region[3 * PAGE] = 42;              /* should trip a WP fault */
    printf("writer: write returned (fault resolved)\n");
    return NULL;
}

int main(void) {
    uffd = (int)syscall(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (uffd < 0) { perror("userfaultfd"); return 1; }

    struct uffdio_api api;
    memset(&api, 0, sizeof api);
    api.api = UFFD_API;
    api.features = UFFD_FEATURE_PAGEFAULT_FLAG_WP;
    if (ioctl(uffd, UFFDIO_API, &api) < 0) { perror("UFFDIO_API"); return 1; }
    printf("features granted: 0x%llx (WP=%d)\n", (unsigned long long)api.features,
           (api.features & UFFD_FEATURE_PAGEFAULT_FLAG_WP) ? 1 : 0);

    region = mmap(NULL, NPAGES * PAGE, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) { perror("mmap"); return 1; }

    /* Variant A: leave pages NON-resident (lazy). This is what the demo does. */
    struct uffdio_register reg;
    memset(&reg, 0, sizeof reg);
    reg.range.start = (uintptr_t)region;
    reg.range.len = NPAGES * PAGE;
    reg.mode = UFFDIO_REGISTER_MODE_WP;
    if (ioctl(uffd, UFFDIO_REGISTER, &reg) < 0) { perror("UFFDIO_REGISTER WP"); return 1; }

    struct uffdio_writeprotect wp;
    memset(&wp, 0, sizeof wp);
    wp.range.start = (uintptr_t)region;
    wp.range.len = NPAGES * PAGE;
    wp.mode = UFFDIO_WRITEPROTECT_MODE_WP;
    if (ioctl(uffd, UFFDIO_WRITEPROTECT, &wp) < 0) { perror("protect UFFDIO_WRITEPROTECT"); return 1; }
    printf("registered + WP-protected %d non-resident pages\n", NPAGES);

    pthread_t mon, wr;
    pthread_create(&mon, NULL, monitor, NULL);
    pthread_create(&wr, NULL, writer, NULL);
    pthread_join(wr, NULL);
    pthread_join(mon, NULL);
    printf("done\n");
    return 0;
}
