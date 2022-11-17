#include <cstdint>
#include <iostream>
#include <vector>

#include "gzipToTrace.h"
/** Not finished */

typedef uint64_t Addr;

const int assoc = 16;   // number of ways
const int blkSize = 64; // bytes

struct OurCacheLine {
    /**
     * An address is devided like this:
     * (MSB) Tag|Index|SetSelect|WordSelect (LSB)
     *
     * Tag is stored in cacheline.
     * Index is in cacheset (the _line array index).
     * SetSelect is in cache (the _set array index).
     * WordSelect is neglected since we look at data at cacheline level.
     */
    Addr tag;
    int timestamp; // global clock tick
    bool valid = false;
};

/**
 * OurCacheSet @{
 */
class OurCacheSet {
    OurCacheLine *_lines;

  public:
    OurCacheSet() { _lines = new OurCacheLine[assoc]; }
    ~OurCacheSet() { delete[] _lines; }
    bool access(Addr tag, int timestamp);

  private:
    int getReplacementWay(); // LRU policy
};

/**
 * @} OurCacheSet
 */
bool OurCacheSet::access(Addr tag, int timestamp) {
    for (int way = 0; way < assoc; ++way) {
        if (_lines[way].tag == tag && _lines[way].valid) {
            // Update timestamp
            _lines[way].timestamp = timestamp;
            return true;
        }
    }

    // If we are here, then it is a miss
    int way = getReplacementWay();
    _lines[way].tag = tag;
    _lines[way].timestamp = timestamp;
    _lines[way].valid = true;

    return false;
}

int OurCacheSet::getReplacementWay() {
    for (int way = 0; way < assoc; ++way) {
        if (!_lines[way].valid) {
            return way;
        }
    }

    // If we are here, then all ways are valid
    int minTime = INT_MAX;
    int minWay = 0;
    for (int way = 0; way < assoc; ++way) {
        if (_lines[way].timestamp < minTime) {
            minWay = way;
            minTime = _lines[way].timestamp;
        }
    }
    return minWay;
}

/**
 * OurCache @{
 */
class OurCache {
    /** We hardcode the block (line) size and associativity */
    int _size;
    int _numSets;
    OurCacheSet *_sets;
    int _globalClock;

    int _numHits;
    int _numMisses;

    int findSet(Addr addr) {
        Addr maskedAddr = addr / blkSize;
        int setIndex = maskedAddr % _numSets;
        return setIndex;
    }

    inline Addr getTag(Addr addr) { return addr / (blkSize * _numSets); }

  public:
    OurCache(int size) : _size(size), _numSets(size / (assoc * blkSize)) {
        _numHits = 0;
        _numMisses = 0;
        _globalClock = 0;
        _sets = new OurCacheSet[_numSets];
        std::cout << "Cache size is " << size << " bytes\n";
    }
    ~OurCache() {}

    /** Read and write operations of the cache. */
    void access(Addr addr);

    int getNumMisses() { return _numMisses; }
    int getNumHits() { return _numHits; }
    int getTotalNumAccess() { return getNumMisses() + getNumHits(); }

    int getSize() { return _size; }
};

/**
 * @} OurCache
 */

void OurCache::access(Addr addr) {
    int setID = findSet(addr);
    Addr tag = getTag(addr);
    bool hit = _sets[setID].access(tag, _globalClock);
    std::cout << "access\n";
    if (hit) {
        _numHits += 1;
    } else {
        _numMisses += 1;
    }
    std::cout << "access, _numHits =  " << _numHits << "\n";
    _globalClock++;
}

int main(int argc, char *argv[]) {
    std::vector<OurCache> caches;
    const int kiloBytes = 1024;
    for (int i = 16; i <= 16 * 1024; i *= 2) {
        OurCache sampleCache(i * kiloBytes);
        caches.push_back(sampleCache);
    }

    std::cout << argv[1] << std::endl;

    int memoryTraceSize;
    addressTrace *memoryTraces = convertGZip2MemoryTraces(argv[1], memoryTraceSize);

    for (int i = 0; i < memoryTraceSize; i++) {
        // std::cout << "Trace[" << i << "] =" << memoryTraces[i] << std::endl;
        for (auto cache : caches) {
            cache.access(memoryTraces[i].address);
        }
    }

    std::cout << "Miss rates are:\n";
    for (auto cache : caches) {
        double missRate = (1.0 * cache.getNumMisses()) / cache.getTotalNumAccess();
        // TODO miss rate
        std::cout << "Cache with " << cache.getSize() << " bytes, missses = " << cache.getNumMisses()
                  << ", total access = " << cache.getTotalNumAccess() << "\n";
    }

    return 0;
}
