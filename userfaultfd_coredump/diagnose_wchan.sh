#!/usr/bin/env bash
# Run the demo, let it hang, then read each thread's kernel wait-channel and
# syscall so we can infer the deadlock without perturbing it with a debugger.
set -u
cd /mnt/e/builds/ion/experiments/uffd_coredump || exit 2

SYSCTL=/proc/sys/vm/unprivileged_userfaultfd
orig=$(cat "$SYSCTL")
echo 1 > "$SYSCTL"

pkill -9 -f uffd_coredump_demo 2>/dev/null
sleep 0.3

./build/uffd_coredump_demo /tmp/ondemand.core 64 4 1 &
pid=$!
echo "demo_pid=$pid"
sleep 4

if kill -0 "$pid" 2>/dev/null; then
    echo "=== hung; per-thread state ==="
    for t in /proc/"$pid"/task/*; do
        tid=${t##*/}
        comm=$(cat "$t/comm" 2>/dev/null)
        state=$(awk '{print $3}' "$t/stat" 2>/dev/null)
        wchan=$(cat "$t/wchan" 2>/dev/null)
        syscall=$(cat "$t/syscall" 2>/dev/null)
        echo "tid=$tid comm=$comm state=$state wchan=[$wchan] syscall=[$syscall]"
    done
    kill -9 "$pid" 2>/dev/null
else
    echo "process already exited (no hang this run)"
fi

echo "$orig" > "$SYSCTL"
echo "done"
