#include <algorithm>
#include <iostream>
#include <map>
#include <math.h>
#include <vector>

#include "gzipToTrace.h"
#include "our_utils.h"

typedef uint64_t TraceNumber;

const int blkSize = 64; // bytes

class StatCache {
    typedef std::map<uint64_t ,uint64_t> ReuseDistHisto;
    typedef std::map<Addr, TraceNumber> MonitoredTrace;
    typedef std::map<Addr, bool> ColdMissTrace;
    /** Each time runTimeStats is called, append to the vectors */
    std::vector<ReuseDistHisto> _reuseDistanceHisto; // <dist, n>
    std::vector<MonitoredTrace> _monitoredTrace;
    std::vector<ColdMissTrace> _coldMissTrace;
    std::vector<uint64_t> _coldMiss;
    std::vector<TraceNumber> _numInst;
    /** Counts how many times runTimeStats is called */
    unsigned _chunkCount = 0;
    std::vector<double> _missRatio;

private:
    /** 
     * A utility function for random sampling 
     * Returns a step S relative to current trace number N. 
     * N + S = next trace number to sample
     */
    inline uint64_t randStep(uint64_t expect, uint64_t maxDiff) {
        return getRandInt(expect - maxDiff, expect + maxDiff);
    }

    /** 
     * f(n) in the paper
     */
    inline double f(double L, double n) {
        return 1 - pow(1 - (1 / L), n);
    }

    inline double solve(double r, unsigned cacheLineNum, int i) {
        assert(i < _chunkCount);
        /** 
         * IMPORTANT:
         * We tried to subtract cold misses from numInst, as a way of ignoring
         *  the cold misses as is described in the paper.
         * It turned out the correct way is to add cold misses to num. This 
         *  gave us a better result.
         */
        double sum = _coldMiss[i];
        for (auto const &distAndCount : _reuseDistanceHisto[i]) {
            uint64_t dist = distAndCount.first;
            // sum += h(K)f(KR)
            sum += distAndCount.second * f((double)cacheLineNum, dist * r);
        }
        return sum / _numInst[i];
    }

public:
    inline uint64_t getColdMiss(int i) { 
        assert(i < _chunkCount);
        return _coldMiss[i];
    }

    inline uint64_t getTotalColdMiss() { 
        uint64_t sum = 0;
        for (uint64_t m : _coldMiss) {
            sum += m;
        }
        return sum;
    }

    inline unsigned getChunkCount() { return _chunkCount; }

    inline double getMissRatio(int i) {
        assert(i < _chunkCount);
        return _missRatio[i];
    }

    inline double getTotalMissRatio() {
        double sum = 0;
        for (double r : _missRatio) {
            sum += r;
        }
        return sum;
    }

    /** To start over from a new analysis */
    void clearStats() {
        // Reset all stats
        _reuseDistanceHisto.clear();
        _monitoredTrace.clear();
        _coldMissTrace.clear();
        _coldMiss.clear();
        _missRatio.clear();
        _chunkCount = 0;
    }
    /** Run time stats on every trace */
    void runTimeStatsFull(std::vector<addressTrace>& trace, 
            TraceNumber start, TraceNumber len);
    /** The post-processing part in the paper. Returns R */
    void postProcessing(unsigned cacheLineNum); 
    void sampleTrace(std::vector<addressTrace>& trace, 
            std::vector<addressTrace>& sampled,
            uint64_t step, uint64_t maxDiff);
};

void StatCache::runTimeStatsFull(std::vector<addressTrace>& trace, 
            TraceNumber start, TraceNumber len) {
    _reuseDistanceHisto.push_back(ReuseDistHisto());
    _monitoredTrace.push_back(MonitoredTrace());
    _coldMissTrace.push_back(ColdMissTrace());
    _coldMiss.push_back(0);
    _numInst.push_back(len);

    /** Make shorter aliases */
    ReuseDistHisto &hist = _reuseDistanceHisto[_chunkCount];
    MonitoredTrace &mt = _monitoredTrace[_chunkCount];
    ColdMissTrace &miss = _coldMissTrace[_chunkCount];
    for (uint64_t i = start; i < start + len; ++i) {
        // Align the addr
        Addr addr = trace[i].address / 64; // block addr -> addr
        if (mt.count(addr)) {
            // Addr is being monitored
            uint64_t dist = i - mt[addr] - 1;
            // Add to histo
            if (hist.count(dist)) {
                hist[dist] += 1;
            } else {
                hist.emplace(dist, 1);
            }
            // Add this addr to monitor
            mt[addr] = i;
        } else {
            // Not monitored. Record cold start stats
            if (!miss.count(addr)) {
                miss.emplace(addr, true);
                _coldMiss[_chunkCount] += 1;
            }
            // Add this addr to monitor
            mt.emplace(addr, i);
        }
    }
    
    _chunkCount += 1;
}

void StatCache::sampleTrace(std::vector<addressTrace>& trace, 
            std::vector<addressTrace>& sampled,
            uint64_t step, uint64_t maxDiff) {
    sampled.clear();
    TraceNumber counter = 0;
    TraceNumber nextSampleTrace = 0;

    while (counter < trace.size()) {
        sampled.push_back(trace[counter]);
        counter += randStep(step, maxDiff); // [step-maxDiff, step+maxDiff]
    }
}

void StatCache::postProcessing(unsigned cacheLineNum) {
    _missRatio.clear();
    for (int i = 0; i < _chunkCount; ++i) {
        _missRatio.push_back(0);

        double r = 0.5;
        double lastR = r;
        const double eps = 0.0001;
        
        while (true) {
            r = solve(r, cacheLineNum, i);
            if (fabs(r - lastR) < eps) {
                break;
            }
            lastR = r;
        }

        _missRatio[i] = r;
    }
}

int main(int argc, char *argv[]) 
{
    // arg: file name
    std::cout << "Arg filename: " << argv[1] << std::endl;
    bool doSample = false; 
    unsigned useTimeSlot = 0;
    if (argc > 2) {
        doSample = atoi(argv[2]);
        std::cout << "Arg sample: " << doSample << std::endl;
    }
    if (argc > 3) {
        useTimeSlot = atoi(argv[3]);
        std::cout << "Arg useTimeSlot: " << useTimeSlot << std::endl;
    }
    
    std::vector<addressTrace> memoryTraces;
    convertGZip2MemoryTraces(argv[1], memoryTraces);
    uint64_t memoryTraceSize = memoryTraces.size();

    StatCache statCache;

    /** Sample the trace */
    std::vector<addressTrace>& sampledTraces = memoryTraces;
    std::vector<addressTrace> sampleContainer;
    if (doSample) {
        uint64_t sampleStep = 100;
        uint64_t sampleStepDiff = 80;
        statCache.sampleTrace(memoryTraces, sampleContainer, 
                    sampleStep, sampleStepDiff);
        // release memory by swapping with an empty vector
        std::vector<addressTrace>().swap(memoryTraces);
        sampledTraces = sampleContainer;
    }
    TraceNumber sampledSize = sampledTraces.size();
    std::cout << "Sample size is " << sampledSize << std::endl;
    
    /** Process traces in time slots */
    unsigned timeSlotLen = sampledSize;
    if (useTimeSlot > 0) {
        timeSlotLen = useTimeSlot;
    }

    /** Get run-time stats from sampled trace */
    statCache.clearStats();
    unsigned nextSlotStart = 0;
    bool finished = false;
    while (!finished) {
        unsigned from = nextSlotStart;
        nextSlotStart += timeSlotLen;
        // last slot cannot be too small
        if (nextSlotStart >= sampledSize
                || sampledSize - nextSlotStart < timeSlotLen / 2) {
            // push one sampledSize at the end
            nextSlotStart = sampledSize;
            finished = true;
        }
        statCache.runTimeStatsFull(sampledTraces, from, nextSlotStart - from);
        //std::cout << "Chunk " << i <<" size " 
        //        << to - from << std::endl;
    }

    std::cout << "Chunk count: " << statCache.getChunkCount() << std::endl;
    std::cout << "Cold misses is " << statCache.getTotalColdMiss() << std::endl;

    /** Post-process trace for all cache sizes from 16KB to 16MB */
    const int kiloBytes = 1024;
    unsigned startSize = 16 * kiloBytes;
    unsigned endSize = 16 * 1024 * kiloBytes;
    std::cout << "Miss rates are:\n";
    for (unsigned size = startSize; size <= endSize; size *= 2) {
        unsigned cacheLineNum = size / blkSize;
        
        statCache.postProcessing(cacheLineNum);

        double r = statCache.getTotalMissRatio();
        r /= statCache.getChunkCount();
        std::cout << "Cache with " << size << " bytes, missses = " 
                << r << std::endl;
        //for (int i = 0; i < statCache.getChunkCount(); ++i) {
        //    std::cout << "\tChunk " << i <<" miss rate " 
        //            << statCache.getMissRatio(i) << std::endl;
        //}
    }

    return 0;
}
