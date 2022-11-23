ifneq ($(DEBUG),)
    CXX_FLAGS += -std=c++11 -O0 -g -Wall
else
    CXX_FLAGS += -std=c++14 -O2 -g -Wall
endif

TARGETS = cacheSimulator statCache

.PHONY: all
all: $(TARGETS)

cacheSimulator : cacheSimulator.cc gzipToTrace.h our_utils.h
	$(CXX) $(CXX_FLAGS) -lboost_iostreams -lz $< -o $@

statCache : statCache.cc gzipToTrace.h our_utils.h
	$(CXX) $(CXX_FLAGS) -lboost_iostreams -lz $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGETS)
