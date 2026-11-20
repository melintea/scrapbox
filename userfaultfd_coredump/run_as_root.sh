#!/usr/bin/env bash
# Temporarily enable unprivileged userfaultfd, run the demo, then restore.
# Intended to be run as root in a disposable dev WSL instance.
set -u
cd /mnt/e/builds/ion/experiments/uffd_coredump || exit 2

SYSCTL=/proc/sys/vm/unprivileged_userfaultfd
orig=$(cat "$SYSCTL")
echo "orig_sysctl=$orig"

echo 1 > "$SYSCTL"
echo "now_sysctl=$(cat "$SYSCTL")"

pkill -9 -f uffd_coredump_demo 2>/dev/null
sleep 0.3
timeout 30 ./build/uffd_coredump_demo /tmp/ondemand.core 64 4 1
rc=$?
echo "demo_exit=$rc (124 == timed out / still hanging)"

echo "$orig" > "$SYSCTL"
echo "restored_sysctl=$(cat "$SYSCTL")"

if [ -f /tmp/ondemand.core ]; then
    echo "core_size_bytes=$(stat -c %s /tmp/ondemand.core)"
fi
exit $rc
