#!/usr/bin/env bash
set -u
cd /mnt/e/builds/ion/experiments/uffd_coredump || exit 2
gcc -O2 -pthread probe_fault.c -o probe_fault || exit 3
SYSCTL=/proc/sys/vm/unprivileged_userfaultfd
orig=$(cat "$SYSCTL")
echo 1 > "$SYSCTL"
timeout 10 ./probe_fault
echo "probe_exit=$?"
echo "$orig" > "$SYSCTL"
