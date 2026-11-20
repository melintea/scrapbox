#!/usr/bin/env bash
set -u
echo 0 > /proc/sys/vm/unprivileged_userfaultfd 2>/dev/null
echo "sysctl_now=$(cat /proc/sys/vm/unprivileged_userfaultfd)"
rm -f /tmp/ondemand.core /tmp/pattern.core
cd /mnt/e/builds/ion/experiments/uffd_coredump || exit 2
g++ -std=c++20 -O2 -g -Wall -Wextra -Wpedantic -pthread \
    -c uffd_coredump.cpp -o /tmp/chk.o
echo "strict_build_exit=$?"
rm -f /tmp/chk.o
