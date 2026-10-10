// pattern_demo.cpp -- fill a buffer with a known byte pattern, dump, and print
// the buffer address so a verifier can read the bytes back out of the core.
//
// Unlike demo_main.cpp this does NOT keep mutating during the dump: it writes a
// fixed pattern, triggers, and the value at trigger time T is unambiguous, so
// the core contents can be compared exactly.

#include "uffd_coredump.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    const std::string out = (argc > 1) ? argv[1] : "/tmp/pattern.core";

    constexpr std::size_t kLen = 8u * 1024 * 1024;  // 8 MiB, comfortably > min_wp
    std::vector<std::uint8_t> buffer(kLen);
    std::memset(buffer.data(), 0xAB, kLen);  // known pattern, all pages resident

    std::printf("BUFADDR=%p\n", static_cast<void*>(buffer.data()));
    std::fflush(stdout);

    uffd_coredump::Dumper dumper;
    if (dumper.Arm() != uffd_coredump::Status::Ok) {
        std::printf("Arm failed: %s\n", dumper.last_error().c_str());
        return 1;
    }
    const auto st = dumper.Trigger(out);
    dumper.Shutdown();
    if (st != uffd_coredump::Status::Ok) {
        std::printf("Trigger failed: %s (%s)\n",
                    uffd_coredump::to_string(st), dumper.last_error().c_str());
        return 1;
    }
    std::printf("dumped ok\n");
    return 0;
}
