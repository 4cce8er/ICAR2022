#include <random>

std::random_device ourRandDevice;
std::mt19937 ourGenerator(ourRandDevice()); 
//std::mt19937 randGen();

int getRandInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max + 1); 
    return dist(ourGenerator);
}
