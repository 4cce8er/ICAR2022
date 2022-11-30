#include <iostream>
#include "assert.h"
#include <dlfcn.h>
/** API: https://intel-pcm-api-documentation.github.io/classPCM.html */
#include "cpucounters.h" // Intel PCM
#include "mm_malloc.h"

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
        _data = (uint8_t*)_mm_malloc(_cacheSize, _waySize);
    }

    ~CachePirate() {
        _mm_free(_data);
    }

    void pirate(unsigned waysToSteal) {
        assert(waysToSteal < _cacheAssoc);
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
/*
	void * handle = dlopen("libpcm.so", RTLD_LAZY);
	if(!handle) {
		printf("Abort: could not (dynamically) load shared library \n");
		return -1;
	}

	pcm::PCM.pcm_c_build_core_event = (int (*)(uint8_t, const char *)) dlsym(handle, "pcm_c_build_core_event");
	pcm::PCM.pcm_c_init = (int (*)()) dlsym(handle, "pcm_c_init");
	pcm::PCM.pcm_c_start = (void (*)()) dlsym(handle, "pcm_c_start");
	pcm::PCM.pcm_c_stop = (void (*)()) dlsym(handle, "pcm_c_stop");
	pcm::PCM.pcm_c_get_cycles = (uint64_t (*)(uint32_t)) dlsym(handle, "pcm_c_get_cycles");
	pcm::PCM.pcm_c_get_instr = (uint64_t (*)(uint32_t)) dlsym(handle, "pcm_c_get_instr");
	pcm::PCM.pcm_c_get_core_event = (uint64_t (*)(uint32_t,uint32_t)) dlsym(handle, "pcm_c_get_core_event");


	if(pcm::PCM.pcm_c_init == NULL || PCM.pcm_c_start == NULL || PCM.pcm_c_stop == NULL ||
			PCM.pcm_c_get_cycles == NULL || PCM.pcm_c_get_instr == NULL ||
			PCM.pcm_c_build_core_event == NULL || PCM.pcm_c_get_core_event == NULL)
		return -1;

    pcm::pcm_c_init();
*/
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