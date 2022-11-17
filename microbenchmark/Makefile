# See LICENSE.txt for license details.

ifneq ($(DEBUG),)
    CXX_FLAGS += -std=c++11 -O0 -g -Wall
else
    CXX_FLAGS += -std=c++11 -O2 -g -Wall
endif

KERNELS = gzipToTrace icar
SUITE = $(KERNELS) #converter

.PHONY: all
all: $(SUITE)

gzipToTrace : gzipToTrace.cc
	$(CXX) $(CXX_FLAGS) -lboost_iostreams -lz  $< -o $@

icar : icar.cc
	$(CXX) $(CXX_FLAGS)  $< -o $@

# Testing
#include test/test.mk

# Benchmark Automation
#include benchmark/bench.mk
#include benchmark/bench_medium.mk


.PHONY: clean
clean:
	rm -f $(SUITE) test/out/*
