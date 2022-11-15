#include <iostream>
#include "mm_malloc.h"

int main(int argc, char *argv[])
{
    int work_iteration = 10;

    const int full_size = 32 * 1024 * 1024; /* Full iteration is 32 MB */
    const int part_size = 128 * 1024;       /* A partial-iteration of 128 KB */
    const int stride = 32;                  /* 2 elements used per line */
    volatile unsigned char* array = \
        (unsigned char*)_mm_malloc(full_size * sizeof(unsigned char), 64);

    unsigned char container;
    int i, j;
    const int last_i = full_size;
    const int part_last_i = part_size;

    for (int iteration = 0; iteration < 10000; iteration++) {
        i = 0;
        while (i < last_i) {
            /* Do the full iteration on odd number lines */
            container = array[i];
            container = array[i + stride];
            
            /* For even number lines, don't go beyond 128KB */
            j = i % part_last_i;
            container = array[j + 2 * stride];
            container = array[j + 3 * stride];
            
            //std::cout << i << " iteration: [" << i << ", " << i + 3 * stride << ")" << std::endl;
            i += 4 * stride;
        }
    }

    _mm_free((void* )array);

    return 0;
}
