#include <algorithm>
#include <iostream>
#include <map>
#include <math.h>
#include <vector>

#include "gzipToTrace.h"
#include "our_utils.h"

typedef TraceNumber uint64_t;

const int blkSize = 64; // bytes

class StatCache {
    /** The histogram we want from run time stats */
    map<uint64_t ,uint64_t> reuseDistanceHisto; // <distance, count>
    /** Below are stats used in one round of analysis of trace 
     * @{
     */
    map<Addr, TraceNumber> monitoredTrace;
    map<Addr, bool> coldMissTrace;
    uint64_t coldMiss;
    /** 
     * @}
     */

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

    inline double solve(double r, TraceNumber numInst, unsigned cacheLineNum) {
        // Optional: count cold misses
        double sum = 0;
        for (auto const &distAndCount : reuseDistanceHisto) {
            uint64_t dist = distAndCount.first;
            // sum += h(K)f(KR)
            sum += distAndCount.second * f((double)cacheLineNum, dist * r);
        }
        return sum / numInst;
    }

public:
    /** To start over from a new analysis */
    void clear() {
        // Reset all stats
        reuseDistanceHisto.clear();
        monitoredTrace.clear();
        coldMissTrace.clear();
        coldMiss = 0;
    }
    /** Run time stats on every trace */
    void runTimeStatsFull(Addr* trace, uint64_t size);
    /** The post-processing part in the paper. Returns R */
    double postProcessing(TraceNumber numInst, unsigned cacheLineSize); 
    void sampleTrace(std::vector<addressTrace>& trace, 
            std::vector<addressTrace>& sampled,
            uint64_t step, uint64_t maxDiff);
}

void StatCache::runTimeStatsFull(std::vector<addressTrace>& trace, 
            TraceNumber start, TraceNumber len) {
    const vector<TraceNumber> &mt = monitoredTrace; // An alias, but shorter
    for (uint64_t i = start; i < start + len; ++i) {
        Addr addr = trace[i];
        if (mt.count(addr)) {
            // Addr is being monitored
            uint64_t dist = i - mt[addr] - 1;
            // Add to histo
            if (reuseDistanceHisto.count(dist)) {
                reuseDistanceHisto[dist] += 1;
            } else {
                reuseDistanceHisto.emplace(dist, 1);
            }
            // Add this addr to monitor
            mt[addr] = i;
        } else {
            // Not monitored. Record cold start stats
            if (!coldMissTrace.count(addr)) {
                coldMissTrace.emplace(addr, True);
                coldMiss += 1;
            }
            // Add this addr to monitor
            mt.emplace(addr, i);
        }
    }
}

void StatCache::sampleTrace(std::vector<addressTrace>& trace, 
            std::vector<addressTrace>& sampled,
            uint64_t step, uint64_t maxDiff) {
    sampled.clear();
    TraceNumber counter = 0;
    TraceNumber nextSampleTrace = 0;

    while (counter < trace.size()) {
        sampled.emplace(trace[counter]);
        counter += randStep(step, maxDiff); // [step-maxDiff, step+maxDiff]
    }
}

double StatCache::postProcessing(TraceNumber numInst, unsigned cacheLineNum) {
    // Iterate cache size from 16KB to 16MB
    double r = 0.5;
    double lastR = r;
    const double eps = 0.0001;
    
    while (true) {
        r = solve(r, numInst, cacheLineNum);
        if (fabs(r - lastR) < eps) {
            break;
        }
        lastR = r;
    }
    std::cout << "Cache with " << size << " bytes, misss rate = " 
            << r << std::endl;
    
    return r;
}

int main(int argc, char *argv[]) 
{
    // arg: file name
    std::cout << argv[1] << std::endl;
    
    std::vector<addressTrace> memoryTraces;
    convertGZip2MemoryTraces(argv[1], memoryTraces);
    uint64_t memoryTraceSize = memoryTraces.size();
    
    StatCache statCache;

    // sample trace
    std::vector<addressTrace> sampledTraces;
    statCache.clear();
    statCache.sampleTrace(memoryTraces, sampledTraces);

    // Get run-time stats from sampled trace
    statCache.runTimeStatsFull(sampledTraces, 0, sampledTraces.size());

    // Post-process trace for all cache sizes from 16KB to 16MB
    const int kiloBytes = 1024;
    unsigned startSize = 16 * kiloBytes;
    unsigned endSize = 16 * 1024 * kiloBytes;
    std::cout << "Miss rates are:\n";
    for (unsigned size = startSize; size <= endSize; size *= 2) {
        unsigned cacheLineNum = size / blkSize;
        double r = 0;
        r = statCache.postProcessing(
                    sampledTraces.size() - statCache.coldMiss, cacheLineNum);
        std::cout << "Cache with " << size << " bytes, missses = " 
                    << r << std::endl;
    }

    return 0;
}