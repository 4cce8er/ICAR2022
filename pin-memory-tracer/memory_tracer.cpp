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

const uint64_t SKIP_INST_LIMIT = 10000000;
const uint64_t ACCESS_LIMIT = UINT64_MAX >> 32;

bool isROI = true;

uint64_t count = 0;
uint64_t accessCount = 0;
uint64_t roiInstCount = 0;

string outfile_basename;

class address_trace {
  public:
    UINT64 address;

    friend std::ostream &operator<<(std::ostream &outs, const address_trace &trace) {
        return outs << std::hex << trace.address << std::dec;
    }
};

std::vector<address_trace> traces;
ofstream *outFileInsCount = NULL;

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
        address_trace current_trace;
        current_trace.address = (UINT64)addr;
        traces.push_back(current_trace);
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
    std::cout << "size of the trace is " << traces.size() << std::endl;
    size_t binary_data_length = traces.size() * sizeof(address_trace);

    char *bytes = new char[traces.size() * sizeof(address_trace)];
    char *begin = reinterpret_cast<char *>(&traces[0]);
    std::memcpy(bytes, begin, traces.size() * sizeof(address_trace));

    size_t n = fwrite(bytes, 1, binary_data_length, stderr);
    fflush(stderr);

    assert(n == binary_data_length);

    // std::cout << bytes;

    *(outFileInsCount) << roiInstCount << endl;
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

    outFileInsCount = new std::ofstream(outfile_basename + "_instCount.txt");

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
