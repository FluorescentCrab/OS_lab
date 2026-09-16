CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra

# Targets without cpu.hpp/cpu.cpp
SIMPLE_TARGETS := fifo round_robin mlfq

# Targets with cpu.hpp/cpu.cpp
MP_TARGETS := fifo_mp round_robin_mp mlfq_mp

ALL_TARGETS := $(SIMPLE_TARGETS) $(MP_TARGETS)
ALL_BINARIES := $(addsuffix .out,$(ALL_TARGETS))

WORKLOAD := testcases/process1.txt
PROCS := 2

.PHONY: all clean run-fifo run-round_robin run-mlfq

all: $(ALL_BINARIES)

# Simple targets: <name>.cpp processes.hpp processes.cpp -> <name>.out
$(addsuffix .out,$(SIMPLE_TARGETS)): %.out: %.cpp processes.hpp processes.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# MP targets: <name>.cpp processes.hpp processes.cpp cpu.hpp cpu.cpp -> <name>.out
$(addsuffix .out,$(MP_TARGETS)): %.out: %.cpp processes.hpp processes.cpp cpu.hpp cpu.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# Convenience run targets — all use the _mp binaries
run-fifo: fifo_mp.out
	./fifo_mp.out $(WORKLOAD) $(PROCS)

run-round_robin: round_robin_mp.out
	./round_robin_mp.out $(WORKLOAD) 4 $(PROCS)

run-mlfq: mlfq_mp.out
	./mlfq_mp.out $(WORKLOAD) 4 3 20 $(PROCS)

clean:
	rm -f $(ALL_BINARIES)
