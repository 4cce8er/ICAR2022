#include <iostream>
#include "assert.h"
/** API: https://intel-pcm-api-documentation.github.io/classPCM.html */
#include "cpucounters.h" // Intel PCM
#include "mm_malloc.h"
#include <sys/mman.h>

class CachePirate {
    const unsigned _blkSize = 64;

    unsigned _cacheSize;
    unsigned _cacheAssoc;
    uint8_t* _data;
    unsigned _waySize;

public:
    CachePirate(unsigned size, unsigned assoc) 
    : _cacheSize(size), _cacheAssoc(assoc) {
        _waySize = _cacheSize / _cacheAssoc;
        _data = (uint8_t*)mmap(NULL, 4 * (1 << 21), PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
                 -1, 0);
    }

    ~CachePirate() {
        //_mm_free(_data);
    }

    void pirate(unsigned waysToSteal) {
        assert(waysToSteal <= _cacheAssoc);
        volatile uint8_t* data = (volatile uint8_t*)_data;
        char discard __attribute__((unused));
        for (unsigned i = 0; i < _waySize; i += _blkSize) {
            for (unsigned j = 0; j < waysToSteal; ++j) {
                discard = data[i + j * _waySize];
            }
        }
    }
};

int main(int argc, char* argv[]) 
{
    /** Initialize PCM */
    pcm::PCM * m = pcm::PCM::getInstance();
    m->program(pcm::PCM::DEFAULT_EVENTS, NULL);

    if (m->program() != pcm::PCM::Success) return -1; 

    //const auto cpu_model = pcm::PCM::getInstance()->getCPUModel();
    pcm::SystemCounterState beforeState = pcm::getSystemCounterState();

    CachePirate cp(8 * 1024 * 1024, 16);
    for (int i = 0; i < 10000; ++i) {
        cp.pirate(8);
    }

    pcm::SystemCounterState afterState = pcm::getSystemCounterState();
    uint64_t l3Misses = pcm::getL3CacheMisses(beforeState, afterState);
    uint64_t l3Hits = pcm::getL3CacheHits(beforeState, afterState);
    uint64_t cycles = pcm::getCycles(beforeState, afterState);

    std::cout << l3Misses << std::endl;
    std::cout << l3Hits << std::endl;
    std::cout << cycles << std::endl;
    return 0;
}