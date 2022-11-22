#include <random>

typedef uint64_t Addr;
typedef uint64_t TimeStamp;

std::random_device ourRandDevice;
std::mt19937 ourGenerator(ourRandDevice()); 

int getRandInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max + 1); 
    return dist(ourGenerator);
}
