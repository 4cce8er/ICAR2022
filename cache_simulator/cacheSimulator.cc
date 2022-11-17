#include <iostream>

#include "our_utils.h"
#include "gzipToTrace.h"
/** Not finished */

typedef uint64_t Addr;
typedef uint64_t TimeStamp;

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
    TimeStamp time;
    bool valid = false;
}

/**
 * OurCacheSet @{
 */
class OurCacheSet {
    int _len;
    OurCacheLine* _line;
public:
    OurCacheSet(int len) : _len(len){ _line = new OurCacheLine[_len]; }
    ~OurCacheSet() { delete[] _line; }
    bool read(int index, Addr tag, TimeStamp time);
    bool write(int index, Addr tag, TimeStamp time);
};

bool OurCacheSet::read(int index, Addr tag, TimeStamp time) {
    bool hit = false;
    if (_line[index].valid && _line[index].tag == tag) {
        hit = true;
    }
    _line[index].time = time;
    if (!hit) {
        _line[index].valid = true;
        _line[index].tag = tag;
    }
    return hit;
}

bool OurCacheSet::write(int index, Addr tag, TimeStamp time) {
    // Exactly the same as read()
    bool hit = false;
    if (_line[index].valid && _line[index].tag == tag) {
        hit = true;
    }
    _line[index].time = time;
    if (!hit) {
        _line[index].valid = true;
        _line[index].tag = tag;
    }
    return hit;
}
/**
 * @} OurCacheSet
 */

/**
 * OurCache @{
 */
class OurCache {
    /** We hardcode the block (line) size and associativity */
    const int _size;
    const int _setLength;
    const int _assoc = 16;
    const int _blkSize = 64;
    OurCacheSet* _set;
    /** Utility functions */
    inline Addr alignToLine(Addr addr) { return addr / _blkSize; }
    inline int selectSet(Addr addr) { return alignToLine(addr) % _assoc; }
    inline int findIndex(Addr addr) { 
        return (addr / (_blkSize * _assoc)) % setLength;
    }
    inline Addr getTag(Addr addr) { 
        return addr / (_blkSize * _assoc * _setLength);
    }
public:
    OurCache(int size) : 
        _size(size), _setLength(size / _assoc / _blkSize) 
        { _set = new OurCacheSet[_assoc](_setLength); }
    OurCache(int size, int blkSize, int assoc) :
        _blkSize(blkSize), _assoc(assoc) { OurCache(size); }
    ~OurCache() { delete [] _set; }

    /** Read and write operations of the cache. Returns miss/hit. */
    bool read(Addr addr);
    bool write(Addr addr);
};

bool OurCache::read(Addr addr) {
    // select sets, and call the read function of CacheSet and stuff
    return true;
}

bool OurCache::write(Addr addr) {
    return true;
}
/**
 * @} OurCache
 */

int main(int argc, char *argv[])
{
    return 0;
}
