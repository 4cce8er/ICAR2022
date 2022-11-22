#include <random>

static class RandomIntGen {
    static std::random_device randomDevice;
    static std::mt19937 randomEngine(randomDevice()); 

    static int get(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max + 1); 
        return randomEngine(dist);
    }
}