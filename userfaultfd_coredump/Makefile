# Makefile for the userfaultfd on-demand core-dump experiment.
#
# Linux-only in substance; builds everywhere but the dumper is a no-op off Linux.
# A plain alternative to the CMake build -- same artifacts, no configure step.
#
# make run/verify still need the vm.unprivileged_userfaultfd=1 sysctl 
# (or CAP_SYS_PTRACE) on a hardened kernel
#
#   make            # build library + demos + probe
#   make demo       # just the write-pressure demo
#   make run        # run the demo (needs root for the sysctl on hardened kernels)
#   make verify     # structural + contents checks via the helper scripts
#   make clean      # remove build artifacts

CXX      ?= g++
CC       ?= gcc
CXXFLAGS ?= -std=c++20 -O2 -g -Wall -Wextra -Wpedantic
CFLAGS   ?= -O2 -g -Wall -Wextra
LDFLAGS  ?= -pthread

LIB      := libuffd_coredump.a
LIB_OBJ  := uffd_coredump.o

DEMO         := uffd_coredump_demo
PATTERN_DEMO := pattern_demo
PROBE        := probe_fault

BINS := $(DEMO) $(PATTERN_DEMO) $(PROBE)

.PHONY: all
all: $(LIB) $(BINS)

# --- static library ---------------------------------------------------------
$(LIB): $(LIB_OBJ)
	$(AR) rcs $@ $^

$(LIB_OBJ): uffd_coredump.cpp uffd_coredump.h
	$(CXX) $(CXXFLAGS) -c uffd_coredump.cpp -o $@

# --- executables ------------------------------------------------------------
$(DEMO): demo_main.cpp $(LIB) uffd_coredump.h
	$(CXX) $(CXXFLAGS) demo_main.cpp $(LIB) -o $@ $(LDFLAGS)

$(PATTERN_DEMO): pattern_demo.cpp $(LIB) uffd_coredump.h
	$(CXX) $(CXXFLAGS) pattern_demo.cpp $(LIB) -o $@ $(LDFLAGS)

# The probe is a self-contained C program with no dependency on the library.
$(PROBE): probe_fault.c
	$(CC) $(CFLAGS) probe_fault.c -o $@ $(LDFLAGS)

# --- convenience targets ----------------------------------------------------
.PHONY: demo
demo: $(DEMO)

.PHONY: run
run: $(DEMO)
	./$(DEMO) /tmp/ondemand.core 64 4 1

.PHONY: probe
probe: $(PROBE)
	./$(PROBE)

.PHONY: verify
verify: $(DEMO) $(PATTERN_DEMO)
	bash inspect_core.sh
	bash verify_contents.sh

.PHONY: clean
clean:
	$(RM) $(LIB) $(LIB_OBJ) $(BINS) /tmp/chk.o
