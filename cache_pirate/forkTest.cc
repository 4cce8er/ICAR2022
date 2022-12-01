/** API: https://intel-pcm-api-documentation.github.io/classPCM.html */
#include "cpucounters.h" // Intel PCM
#include "mm_malloc.h"
#include <sched.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

class CachePirate
{
    const unsigned _blkSize = 64;

    unsigned _cacheSize;
    unsigned _cacheAssoc;
    uint8_t *_data;
    unsigned _waySize;

public:
    CachePirate(unsigned size, unsigned assoc)
        : _cacheSize(size), _cacheAssoc(assoc), 
    {
        _waySize = _cacheSize / _cacheAssoc;
        //_data = (uint8_t*)_mm_malloc(_cacheSize, _waySize);
        _data = (uint8_t*)mmap(NULL, 4 * (1 << 21), PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
                 -1, 0);
    }

    ~CachePirate()
    {
        //_mm_free(_data);
        munmap(_data, 4 * (1 << 21));
    }

    void pirate(unsigned waysToSteal)
    {
        assert(waysToSteal <= _cacheAssoc);
        volatile uint8_t *data = (volatile uint8_t *)_data;
        char discard __attribute__((unused));

        // TODO omp for multithread pirate

        for (unsigned i = 0; i < _waySize; i += _blkSize)
        {
            for (unsigned j = 0; j < waysToSteal; ++j)
            {
                discard = data[i + j * _waySize];
            }
        }
    }
};

int main(int argc, char *argv[])
{
    unsigned stealWayNum = 0;
    // argv = [forkTest, 8, benchmark, benchmarkArg0, benchmarkArgu1, ....]
    if (argc < 3)
    {
        std::cout << "Args: stealWayNum, benchmarkName, benchmarkArg1, ..." << std::endl;
        exit(-1);
    }

    std::cout << "Arg benchmarkName: " << argv[2] << std::endl;

    stealWayNum = atoi(argv[1]);
    std::cout << "Arg stealWayNum: " << stealWayNum << std::endl;

    pid_t childPid = fork();

    if (childPid == 0)
    { // Child is the target

        cpu_set_t my_set;
        CPU_ZERO(&my_set);   /* Initialize it all to 0, i.e. no CPUs selected. */
        CPU_SET(0, &my_set); /* set the bit that represents core 0. */
        int result = sched_setaffinity(getpid(), sizeof(cpu_set_t),
                                       &my_set); // Set affinity of tihs process to  the defined mask, i.e. only 0
        if (result != 0)
        {
            std::cout << "sched_setaffinity did not succeed" << std::endl;
        }
        std::cout << "Child pid = " << getpid() << ", cpu = " << CPU_COUNT(&my_set)
                  << ", sched_getcpu() = " << sched_getcpu() << std::endl;

        if (-1 == execve(argv[2], (char **)&argv[2], NULL))
        {
            perror("child process execve failed [%m]");
            return -1;
        }

        // std::string targetProgramStr = argv[2];
        // for(int i = 3; i < argc; i++){
        //     targetProgramStr += " " + std::string(argv[i]);
        // }
        // system(targetProgramStr.c_str());

        std::cout << "Target call completed\n";
        exit(1);
    }
    else
    {
        pcm::PCM *m = pcm::PCM::getInstance();
        m->program(pcm::PCM::DEFAULT_EVENTS, NULL);

        cpu_set_t my_set;
        int result = sched_getaffinity(getpid(), sizeof(my_set), &my_set);
        if (result != 0)
        {
            std::cout << "sched_getaffinity did not succeed" << std::endl;
        }
        std::cout << "Parent pid = " << getpid() << ", cpu = " << CPU_COUNT(&my_set)
                  << ", sched_getcpu() = " << sched_getcpu() << std::endl;
        CPU_CLR(0, &my_set); /* remove core CPU zero form parent/pirate process */
        result = sched_setaffinity(getpid(), sizeof(cpu_set_t),
                                   &my_set); // Set affinity of tihs process to  the defined mask, i.e. only 0
        if (result != 0)
        {
            std::cout << "sched_setaffinity did not succeed" << std::endl;
        }
        std::cout << "Parent pid = " << getpid() << ", cpu = " << CPU_COUNT(&my_set)
                  << ", sched_getcpu() = " << sched_getcpu() << std::endl;
        /** Fork a pirate process so we can kill it when we need */
        pid_t piratePid = fork();

        if (piratePid == 0)
        {
            // pcm::PCM *m = pcm::PCM::getInstance();
            // m->program(pcm::PCM::DEFAULT_EVENTS, NULL);

            // if (m->program() != pcm::PCM::Success) {
            //     return -1;
            // }
            CachePirate cp(8 * 1024 * 1024, 16);
            while (true)
            {
                // pcm::SystemCounterState beforeState = pcm::getSystemCounterState();
                // Call the pirate
                cp.pirate(stealWayNum);
                // pcm::SystemCounterState afterState = pcm::getSystemCounterState();
                // uint64_t l3Misses = pcm::getL3CacheMisses(beforeState, afterState);
                // uint64_t l3Hits = pcm::getL3CacheHits(beforeState, afterState);
                // uint64_t cycles = pcm::getCycles(beforeState, afterState);

                // std::cout << "l3Misses = " << l3Misses << std::endl;
                // std::cout << "l3Hits = " << l3Hits << std::endl;
                // std::cout << "cyles = " << cycles << std::endl;
            }
        }
        else
        {
            // pcm::PCM *m = pcm::PCM::getInstance();
            // m->program(pcm::PCM::DEFAULT_EVENTS, NULL);

            const int coreNum = 4;
            pcm::CoreCounterState coreBeforeStates[coreNum];
            pcm::CoreCounterState coreAfterStates[coreNum];
            for (int i = 0; i < coreNum; i++)
            {
                coreBeforeStates[i] = pcm::getCoreCounterState(i);
            }

            // pcm::CoreCounterState beforeState = pcm::getCoreCounterState(0);

            pcm::SystemCounterState systemBeforeState = pcm::getSystemCounterState();

            std::cout << "Waiting the target...\n";
            int status;
            waitpid(childPid, &status, WUNTRACED);
            std::cout << "Child returned\n";

            // Kill the pirate
            kill(piratePid, SIGKILL);

            uint64_t core_l3Misses = 0;
            uint64_t core_l3Hit = 0;
            uint64_t core_cycles = 0;

            for (int i = 0; i < coreNum; i++)
            {
                coreAfterStates[i] = pcm::getCoreCounterState(i);
            }

            for (int i = 0; i < coreNum; i++)
            {
                uint64_t curCore_l3Misses = pcm::getL3CacheMisses(coreBeforeStates[i], coreAfterStates[i]);
                uint64_t curCore_l3Hits = pcm::getL3CacheHits(coreBeforeStates[i], coreAfterStates[i]);
                uint64_t curCore_cycles = pcm::getCycles(coreBeforeStates[i], coreAfterStates[i]);

                std::cout << "core " << i << " l3Misses = " << curCore_l3Misses << std::endl;
                std::cout << "core " << i << " l3Hits = " << curCore_l3Hits << std::endl;
                std::cout << "core " << i << " cyles = " << curCore_cycles << std::endl;

                core_l3Misses += curCore_l3Misses;
                core_l3Hit += curCore_l3Hits;
                core_cycles += curCore_cycles;
            }

            pcm::SystemCounterState systemAfterState = pcm::getSystemCounterState();
            uint64_t sys_l3Misses = pcm::getL3CacheMisses(systemBeforeState, systemAfterState);
            uint64_t sys_l3Hits = pcm::getL3CacheHits(systemBeforeState, systemAfterState);
            uint64_t sys_cycles = pcm::getCycles(systemBeforeState, systemAfterState);

            std::cout << "System l3Misses = " << sys_l3Misses << std::endl;
            std::cout << "System l3Hits = " << sys_l3Hits << std::endl;
            std::cout << "System  cyles = " << sys_cycles << std::endl;

            std::cout << "core l3Misses = " << core_l3Misses << std::endl;
            std::cout << "core l3Hits = " << core_l3Hit << std::endl;
            std::cout << "core  cyles = " << core_cycles << std::endl;
        }
    }

    return 0;
}
