#!/usr/bin/env bash
# Validate the produced core with readelf and gdb.
set -u
CORE=/tmp/ondemand.core
BIN=/mnt/e/builds/ion/experiments/uffd_coredump/build/uffd_coredump_demo

echo "=== readelf -h (ELF header) ==="
readelf -h "$CORE" 2>&1 | sed -n '1,20p'

echo "=== readelf -l (program headers) ==="
readelf -l "$CORE" 2>&1 | sed -n '1,25p'

echo "=== LOAD segment count ==="
readelf -l "$CORE" 2>&1 | grep -c LOAD

echo "=== gdb: open core, verify it maps a known region ==="
# The demo prints the buffer address; we just confirm gdb can read some memory
# out of the core via its PT_LOAD segments without erroring on structure.
gdb -q -batch \
    -ex "set pagination off" \
    -ex "core-file $CORE" \
    -ex "info files" 2>&1 | sed -n '1,40p'
