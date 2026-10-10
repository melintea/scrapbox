// demo_main.cpp -- exercise uffd_coredump under write pressure.
//
// Allocates a large mutable buffer, spins mutator threads that hammer it, and
// triggers an on-demand dump mid-flight so the WP-fault preservation path is
// actually exercised. Then it sanity-checks that the output is an ELF core.
//
//   ./uffd_coredump_demo [out_path] [buffer_mib] [mutator_threads] [seconds]

#include "uffd_coredump.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

std::atomic<bool> g_stop{false};

void mutator(std::uint8_t* buf, std::size_t len, unsigned seed)
{
    std::uint64_t x = 0x9E3779B97F4A7C15ull ^ seed;  // xorshift-ish PRNG
    while (!g_stop.load(std::memory_order_relaxed)) {
        x ^= x << 13; x ^= x >> 7; x ^= x << 17;
        const std::size_t idx = static_cast<std::size_t>(x % len);
        buf[idx] = static_cast<std::uint8_t>(x);  // write => may trip a WP fault
    }
}

bool looks_like_elf_core(const std::string& path)
{
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) { return false; }
    unsigned char id[20] = {0};
    const std::size_t n = std::fread(id, 1, sizeof id, f);
    std::fclose(f);
    if (n < 18) { return false; }
    const bool magic = id[0] == 0x7f && id[1] == 'E' && id[2] == 'L' && id[3] == 'F';
    const bool is_core = id[16] == 4;  // ET_CORE low byte (e_type, little-endian)
    return magic && is_core;
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string out_path     = (argc > 1) ? argv[1] : "/tmp/ondemand.core";
    const std::size_t buffer_mib   = (argc > 2) ? std::strtoul(argv[2], nullptr, 10) : 256;
    const unsigned    n_mutators   = (argc > 3) ? std::strtoul(argv[3], nullptr, 10) : 4;
    const unsigned    run_seconds  = (argc > 4) ? std::strtoul(argv[4], nullptr, 10) : 2;

    if (!uffd_coredump::Dumper::PlatformSupported()) {
        std::printf("userfaultfd core dump is not supported on this platform; skipping.\n");
        return 0;
    }

    const std::size_t len = buffer_mib * 1024u * 1024u;
    std::vector<std::uint8_t> buffer(len, 0u);
    std::printf("allocated %zu MiB mutable buffer at %p\n", buffer_mib,
                static_cast<void*>(buffer.data()));

    uffd_coredump::Dumper dumper;
    uffd_coredump::Options opts;
    auto st = dumper.Arm(opts);
    if (st != uffd_coredump::Status::Ok) {
        std::printf("Arm() failed: %s (%s)\n",
                    uffd_coredump::to_string(st), dumper.last_error().c_str());
        return 1;
    }
    std::printf("armed.\n");

    std::vector<std::thread> threads;
    threads.reserve(n_mutators);
    for (unsigned i = 0; i < n_mutators; ++i) {
        threads.emplace_back(mutator, buffer.data(), len, i + 1);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::printf("triggering dump to %s while %u threads write...\n",
                out_path.c_str(), n_mutators);

    const auto t0 = std::chrono::steady_clock::now();
    st = dumper.Trigger(out_path);
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::this_thread::sleep_for(std::chrono::seconds(run_seconds));
    g_stop.store(true, std::memory_order_relaxed);
    for (auto& t : threads) { t.join(); }

    dumper.Shutdown();

    if (st != uffd_coredump::Status::Ok) {
        std::printf("Trigger() failed: %s (%s)\n",
                    uffd_coredump::to_string(st), dumper.last_error().c_str());
        return 1;
    }

    std::printf("dump completed in %.1f ms\n", ms);
    if (looks_like_elf_core(out_path)) {
        std::printf("OK: %s is a well-formed ELF core (ET_CORE).\n", out_path.c_str());
        std::printf("verify with: eu-readelf -n %s   or   gdb <exe> %s\n",
                    out_path.c_str(), out_path.c_str());
        return 0;
    }
    std::printf("FAIL: %s does not look like an ELF core.\n", out_path.c_str());
    return 1;
}
