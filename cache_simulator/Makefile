ifneq ($(DEBUG),)
    CXX_FLAGS += -std=c++11 -O0 -g -Wall
else
    CXX_FLAGS += -std=c++11 -O2 -g -Wall
endif

KERNELS = cacheSimulator
SUITE = $(KERNELS) #converter

.PHONY: all
all: $(SUITE)

cacheSimulator : cacheSimulator.cc gzipToTrace.h our_utils.h 
	$(CXX) $(CXX_FLAGS) -lboost_iostreams -lz $< -o $@

.PHONY: clean
clean:
	rm -f $(SUITE) out/*
