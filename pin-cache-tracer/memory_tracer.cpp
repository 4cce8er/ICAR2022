/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

#include <fstream>
#include <iostream>

#include <vector>

#include "pin.H"
using std::cerr;
using std::cout;
using std::endl;
using std::ofstream;
using std::string;

typedef uint64_t Addr;
typedef uint64_t TimeStamp;

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
    TimeStamp timestamp; // global clock tick
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
    bool access(Addr tag, TimeStamp timestamp);

  private:
    int getReplacementWay(); // LRU policy
};

bool OurCacheSet::access(Addr tag, TimeStamp timestamp) {
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
    TimeStamp minTime = UINT64_MAX;
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
 * @} OurCacheSet
 */

/**
 * OurCache @{
 */
class OurCache {
    /** We hardcode the block (line) size and associativity */
    int _size;
    int _numSets;
    OurCacheSet *_sets;
    TimeStamp _globalClock;

    uin64_t _numHits;
    uin64_t _numMisses;

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

    uint64_t getNumMisses() { return _numMisses; }
    uint64_t getNumHits() { return _numHits; }
    uint64_t getTotalNumAccess() { return getNumMisses() + getNumHits(); }

    int getSize() { return _size; }
};

void OurCache::access(Addr addr) {
    int setID = findSet(addr);
    Addr tag = getTag(addr);
    bool hit = _sets[setID].access(tag, _globalClock);
    if (hit) {
        _numHits += 1;
    } else {
        _numMisses += 1;
    }
    _globalClock++;
}
/**
 * @} OurCache
 */

// const uint64_t SKIP_INST_LIMIT = 100000000;
const uint64_t SKIP_INST_LIMIT = 10000000;
const uint64_t ACCESS_LIMIT = UINT64_MAX >> 32;

bool isROI = true;

uint64_t count = 0;
uint64_t accessCount = 0;
uint64_t roiInstCount = 0;

std::vector<OurCache> caches;

// Set ROI flag
void startROI() {
    // std::cout << "Start ROI\n";
    isROI = true;
    count = SKIP_INST_LIMIT;
}

// Set ROI flag
void stopROI() {
    // std::cout << "End ROI\n";
    isROI = false;
}

void routineCallback(RTN rtn, void *v) {
    // std::string rtn_name = RTN_Name(rtn).c_str();
    std::string rtn_name = RTN_Name(rtn);

    if (rtn_name.find("PIN_Start") != std::string::npos) {
        RTN_Replace(rtn, AFUNPTR(startROI));
    }

    if (rtn_name.find("PIN_Stop") != std::string::npos) {
        RTN_Replace(rtn, AFUNPTR(stopROI));
    }
}

VOID PIN_FAST_ANALYSIS_CALL RecordMem(VOID *addr, bool type) {
    if (accessCount < ACCESS_LIMIT) {
        for (auto &cache : caches) {
            cache.access((UINT64)addr);
        }
        accessCount++;
    } else if (accessCount == ACCESS_LIMIT) {
        roiInstCount = count - SKIP_INST_LIMIT;
        PIN_ExitApplication(0);
    }
}

// This function is called before every instruction
VOID PIN_FAST_ANALYSIS_CALL docount() { count += 1; }

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v) {
    // Return if not in ROI
    if (!isROI) {
        return;
    }

    if (!INS_IsSyscall(ins)) {
        if (count >= SKIP_INST_LIMIT) {

            // Instruments memory accesses using a predicated call, i.e.
            // the instrumentation is called iff the instruction will actually be executed.
            //
            // On the IA-32 and Intel(R) 64 architectures conditional moves and REP
            // prefixed instructions appear as predicated instructions in Pin.
            UINT32 memOperands = INS_MemoryOperandCount(ins);

            // Iterate over each memory operand of the instruction.
            for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
                if (INS_MemoryOperandIsRead(ins, memOp)) {
                    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem, IARG_MEMORYOP_EA, memOp, IARG_BOOL,
                                             0, IARG_END);
                }
                // Note that in some architectures a single memory operand can be
                // both read and written (for instance incl (%eax) on IA-32)
                // In that case we instrument it once for read and once for write.
                if (INS_MemoryOperandIsWritten(ins, memOp)) {
                    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem, IARG_MEMORYOP_EA, memOp, IARG_BOOL,
                                             1, IARG_END);
                }
            }
        }

        // begin each instruction with this function
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_END);
    }
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v) {
    std::cout << "Total Number of instructions = " << count << std::endl;
    if (roiInstCount == 0) {
        roiInstCount = count - SKIP_INST_LIMIT;
    }
    std::cout << "Access limit is = " << ACCESS_LIMIT << std::endl;
    std::cout << "Total accesses are = " << accessCount << std::endl;
    std::cout << "ROI Instruction Count = " << roiInstCount << std::endl;
    std::cout << "Miss rates are:\n";
    for (auto &cache : caches) {
        double missRate = (1.0 * cache.getNumMisses()) / cache.getTotalNumAccess();
        std::cout << "Cache with " << cache.getSize() << " bytes, missses = " << cache.getNumMisses()
                  << ", hits = " << cache.getNumHits() << " total accesses = " << cache.getTotalNumAccess()
                  << ", miss rate = " << missRate << "\n";
    }
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage() {
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return 1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[]) {
    // Initialize pin
    PIN_InitSymbols();
    if (PIN_Init(argc, argv))
        return Usage();

    const int kiloBytes = 1024;
    for (int i = 16; i <= 16 * 1024; i *= 2) {
        caches.emplace_back(i * kiloBytes);
    }
    isROI = true;
    count = 0;

    RTN_AddInstrumentFunction(routineCallback, 0);

    // Register Fini to be called when the application exits.
    PIN_AddFiniFunction(Fini, NULL);

    // Register Instruction to be called to instrument instructions.
    INS_AddInstrumentFunction(Instruction, NULL);

    // Start the program, never returns
    PIN_StartProgram();

    return 1;
}
