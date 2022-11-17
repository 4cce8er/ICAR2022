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

addressTrace *convertGZip2MemoryTraces(char *fileName, int &traceSize) {
    std::ifstream file1(fileName, std::ios_base::ate | std::ios_base::binary);
    int bufferSize = file1.tellg();
    file1.close();

    // Read from the first command line argument, assume it's gzipped
    std::ifstream file(fileName, std::ios_base::in | std::ios_base::binary);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
    inbuf.push(boost::iostreams::gzip_decompressor());
    inbuf.push(file);
    // Convert streambuf to istream
    std::istream instream(&inbuf);

    char *output = (char *)malloc(bufferSize);
    instream.rdbuf()->sgetn(reinterpret_cast<char *>(output), bufferSize);

    traceSize = bufferSize / sizeof(addressTrace);
    addressTrace *traces = reinterpret_cast<addressTrace *>(output);

    // Copy everything from instream to
    //  buf.sgetn(reinterpret_cast<char *>(output), buf.size());
    //  std::cout << instream.rdbuf();
    // Cleanup
    file.close();

    return traces;
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