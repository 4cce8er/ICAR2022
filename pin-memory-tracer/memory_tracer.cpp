/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

#include <fstream>
#include <iostream>

#include "pin.H"
using std::cerr;
using std::cout;
using std::endl;
using std::ofstream;
using std::string;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "", "specify output file name");

INT32 numThreads = 0;
uint64_t numInsns{0};
string outfile_basename;

const CHAR *ROI_BEGIN = "__parsec_roi_begin";
const CHAR *ROI_END = "__parsec_roi_end";
bool isROI = true;

class address_trace {
  public:
    UINT64 address;
    bool type;

    friend std::ostream &operator<<(std::ostream &outs, const address_trace &trace) {
        return outs << trace.type << "," << std::hex << trace.address << std::dec;
    }
};

// a running count of the instructions
class thread_data_t {
  public:
    thread_data_t() {
        _count = 0;
        _outFileInsCount = NULL;
    }
    UINT64 _count;
    std::vector<address_trace> _traces;
    ofstream *_outFileInsCount;
};

// key for accessing TLS storage in the threads. initialized once in main()
static TLS_KEY tls_key = INVALID_TLS_KEY;

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v) {
    // printf("thread begin %d\n", threadid);
    numThreads++;
    thread_data_t *tdata = new thread_data_t;
    if (PIN_SetThreadData(tls_key, tdata, threadid) == FALSE) {
        cerr << "PIN_SetThreadData failed" << endl;
        PIN_ExitProcess(1);
    }

    tdata->_outFileInsCount = new std::ofstream(outfile_basename + "_" + std::to_string(threadid) + "_instCount.txt");
}

VOID PIN_FAST_ANALYSIS_CALL RecordMem(VOID *addr, bool type, THREADID threadid) {
    thread_data_t *tdata = static_cast<thread_data_t *>(PIN_GetThreadData(tls_key, threadid));
    address_trace current_trace;
    current_trace.address = (UINT64)addr;
    current_trace.type = type;
    tdata->_traces.push_back(current_trace);
}

// This function is called before every instruction
VOID PIN_FAST_ANALYSIS_CALL docount(THREADID threadid) {
    thread_data_t *tdata = static_cast<thread_data_t *>(PIN_GetThreadData(tls_key, threadid));
    tdata->_count += 1;
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v) {
    // Return if not in ROI
    if (!isROI) {
        return;
    }

    if (!INS_IsSyscall(ins)) {

        // Instruments memory accesses using a predicated call, i.e.
        // the instrumentation is called iff the instruction will actually be executed.
        //
        // On the IA-32 and Intel(R) 64 architectures conditional moves and REP
        // prefixed instructions appear as predicated instructions in Pin.
        UINT32 memOperands = INS_MemoryOperandCount(ins);

        // Iterate over each memory operand of the instruction.
        for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
            if (INS_MemoryOperandIsRead(ins, memOp)) {
                INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem, IARG_MEMORYOP_EA, memOp, IARG_BOOL, 0,
                                         IARG_THREAD_ID, IARG_END);
            }
            // Note that in some architectures a single memory operand can be
            // both read and written (for instance incl (%eax) on IA-32)
            // In that case we instrument it once for read and once for write.
            if (INS_MemoryOperandIsWritten(ins, memOp)) {
                INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem, IARG_MEMORYOP_EA, memOp, IARG_BOOL, 1,
                                         IARG_THREAD_ID, IARG_END);
            }
        }

        // begin each instruction with this function
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_THREAD_ID, IARG_END);
    }
}

// This function is called when the thread exits
VOID ThreadFini(THREADID threadIndex, const CONTEXT *ctxt, INT32 code, VOID *v) {
    // printf("thread end %d code %d\n", threadIndex, code);
    thread_data_t *tdata = static_cast<thread_data_t *>(PIN_GetThreadData(tls_key, threadIndex));
    
    size_t binary_data_length = tdata->_traces.size() * sizeof(address_trace);

    char *bytes = new char[tdata->_traces.size() * sizeof(address_trace)];
    char *begin = reinterpret_cast<char *>(&tdata->_traces[0]);
    std::memcpy(bytes, begin, tdata->_traces.size() * sizeof(address_trace));

    size_t n = fwrite(bytes, 1, binary_data_length, stdout);
    fflush(stdout);

    assert(n == binary_data_length);

    // std::cout << bytes;

    *(tdata->_outFileInsCount) << tdata->_traces.size() << endl;

    delete tdata;
}

// Set ROI flag
VOID StartROI() {
    // std::cout << "Start ROI\n";
    isROI = true;
}

// Set ROI flag
VOID StopROI() {
    // std::cout << "End ROI\n";
    isROI = false;
}

// Pin calls this function every time a new rtn is executed
VOID Routine(RTN rtn, VOID *v) {
    // Get routine name
    const CHAR *name = RTN_Name(rtn).c_str();

    if (strcmp(name, ROI_BEGIN) == 0) {
        // Start tracing after ROI begin exec
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)StartROI, IARG_END);
        RTN_Close(rtn);
    } else if (strcmp(name, ROI_END) == 0) {
        // Stop tracing before ROI end exec
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)StopROI, IARG_END);
        RTN_Close(rtn);
    }
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v) {
    // cout << "Total number of threads = " << numThreads << endl;
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

    outfile_basename = KnobOutputFile.Value().empty() ? "default_traces" : KnobOutputFile.Value().c_str();
    // OutFile = KnobOutputFile.Value().empty() ? &cout : new std::ofstream(KnobOutputFile.Value().c_str());

    // Obtain  a key for TLS storage.
    tls_key = PIN_CreateThreadDataKey(NULL);
    if (tls_key == INVALID_TLS_KEY) {
        cerr << "number of already allocated keys reached the MAX_CLIENT_TLS_KEYS limit" << endl;
        PIN_ExitProcess(1);
    }

    // Register ThreadStart to be called when a thread starts.
    PIN_AddThreadStartFunction(ThreadStart, NULL);

    // Register Fini to be called when thread exits.
    PIN_AddThreadFiniFunction(ThreadFini, NULL);

    // Register Fini to be called when the application exits.
    PIN_AddFiniFunction(Fini, NULL);

    // Register Instruction to be called to instrument instructions.
    INS_AddInstrumentFunction(Instruction, NULL);

    RTN_AddInstrumentFunction(Routine, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 1;
}
