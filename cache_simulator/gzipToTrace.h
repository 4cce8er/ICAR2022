#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <fstream>
#include <iostream>

class addressTrace {
  public:
    uint64_t address;

    friend std::ostream &operator<<(std::ostream &outs, const addressTrace &trace) {
        return outs << std::hex << trace.address << std::dec;
    }
};

void convertGZip2MemoryTraces(char *fileName, std::vector<addressTrace> &traces) {
    // assume it's gzipped
    std::ifstream file(fileName, std::ios_base::in | std::ios_base::binary);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
    inbuf.push(boost::iostreams::gzip_decompressor());
    inbuf.push(file);

    std::vector <char> vecArg;
    vecArg.assign(std::istreambuf_iterator<char>{&inbuf}, {});

    std::cout << "vecArg.size() = " << vecArg.size() << std::endl;

    addressTrace* ptrToByteVector =  reinterpret_cast<addressTrace *>(vecArg.data());

    // There is no (convenient) way to just cast a vector of type A to vector of type B, we need to copy..
    traces.assign(ptrToByteVector, ptrToByteVector + vecArg.size()/sizeof(addressTrace) );

    std::cout << "traces.size() = " << traces.size() << std::endl;

    // Cleanup
    file.close();
}

/*
int main(int argc, char *argv[]) {
    std::cout << argv[1] << std::endl;

    int memoryTraceSize;
    addressTrace *memoryTraces = convertGZip2MemoryTraces(argv[1],memoryTraceSize);

    for (int i = 0; i < memoryTraceSize; i++) {
        std::cout << "Trace[" << i << "] =" << memoryTraces[i] << std::endl;
    }
}
*/