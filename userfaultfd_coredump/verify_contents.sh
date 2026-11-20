#!/usr/bin/env bash
# Prove the core contains real memory contents by:
#  1. running a demo variant that fills a buffer with a known pattern,
#  2. dumping,
#  3. reading those bytes back out of the core with gdb and comparing.
set -u
cd /mnt/e/builds/ion/experiments/uffd_coredump || exit 2

SYSCTL=/proc/sys/vm/unprivileged_userfaultfd
orig=$(cat "$SYSCTL")
echo 1 > "$SYSCTL"

pkill -9 -f pattern_demo 2>/dev/null
sleep 0.2

# Build the pattern demo (writes 0xAB across the buffer, prints its address,
# triggers a dump, then idles so we know the value at T).
g++ -std=c++20 -O2 -g -pthread pattern_demo.cpp uffd_coredump.cpp -o pattern_demo || exit 3

out=$(./pattern_demo /tmp/pattern.core 2>&1)
echo "$out"
addr=$(echo "$out" | sed -n 's/^BUFADDR=//p')
echo "parsed addr=$addr"

echo "=== gdb read 16 bytes at $addr from the core ==="
gdb -q -batch \
    -ex "set pagination off" \
    -ex "core-file /tmp/pattern.core" \
    -ex "x/16xb $addr" 2>&1 | sed -n '1,12p'

echo "$orig" > "$SYSCTL"
