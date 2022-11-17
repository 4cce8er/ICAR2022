#include <iostream>

#include "our_utils.h"
#include "gzipToTrace.h"
/** Not finished */

typedef uint64_t Addr;

class OurCacheSet {
    int _size;
    Addr* _tag;
    bool* _valid;
    // A time tag for LRU? Maybe use a binary tree?
public:
    // Add a function to get index according to Addr
};

class OurCache {
    /** We hardcode the block (line) size and associativity */
    const int _blkSize = 64;
    const int _assoc = 16;
    /** 
     * An address is devided like this:
     * (MSB) Tag|Index|SetSelect|WordSelect (LSB)
     * 
     */
    /** Array of Cache Sets */
    OurCacheSet* _set;
    /** Utility functions */
    inline Addr alignToLine(Addr addr) { return addr / _blkSize; }
    inline int selectSet(Addr addr) { return alignToLine(addr) % _assoc; }
public:
    OurCache();
    OurCache(int blkSize, int assoc) :
        _blkSize(blkSize), _assoc(assoc) { OurCache(); }
    ~OurCache() { delete [] _set; }

    /** Read and write operations of the cache. Returns miss/hit. */
    bool read(Addr addr);
    bool write(Addr addr);
};

OurCache::OurCache() {
    _set = new OurCacheSet[_assoc];
}

bool OurCache::read(Addr addr) {
    // select sets, and call the read function of CacheSet and stuff
    return true;
}

bool OurCache::write(Addr addr) {
    return true;
}

int main(int argc, char *argv[])
{
    return 0;
}
