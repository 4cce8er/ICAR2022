#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

#include "gzipToTrace.h"
#include "our_utils.h"

typedef TraceNumber uint64_t;

class StatCache {
    /** The histogram we want from run time stats */
    map<uint64_t ,uint64_t> reuseDistanceHisto;
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
    /** 
     * Run time stats on sampled trace
     * Sample step is a uniform distribution [step-maxDiff, stemp+maxDiff]
     */
    void runTimeStatsSample(Addr* trace, uint64_t size, 
            uint64_t step, uint64_t maxDiff);
    /** The post-processing part in the paper */
    void postProcessing();  // TODO
}

void StatCache::runTimeStatsFull(Addr* trace, TraceNumber size) {
    const vector<TraceNumber> &mt = monitoredTrace; // An alias, but shorter
    for (uint64_t i = 0; i < size; ++i) {
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

void StatCache::runTimeStatsSample(Addr* trace, uint64_t size, 
            uint64_t step, uint64_t maxDiff) {
    const vector<TraceNumber> &mt = monitoredTrace; // An alias, but shorter
    TraceNumber nextSampleTrace = 0;
    for (uint64_t i = 0; i < size; ++i) {
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
        } else {
            // Not monitored. Record cold start stats
            if (!coldMissTrace.count(addr)) {
                coldMissTrace.emplace(addr, True);
                coldMiss += 1;
            }
        }

        if (i == nextSampleTrace) {
            // Not it's time to add current addr to monitor
            if (mt.count(addr)) {
                mt[addr] = i;
            } else {
                mt.emplace(addr, i);
            }
            // Randomly choose next sampled trace
            nextSampleTrace += randStep(step, maxDiff);
        }
    }
}