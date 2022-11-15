#include <iostream>

int main(int argc, char *argv[])
{
    int work_iteration = 10;

    const int full_size = 16 * 1024 * 1024;   /* Full iteration is 16 MB */
    const int part_size = 128 * 1024;        /* A partial-iteration of 128 KB */
    const int stride = 32;                    /* 2 elements used per line */
    volatile unsigned char* array = new unsigned char[full_size];

    unsigned char container;
    int i;
    const int last_i = full_size;
    const int part_last_i = part_size;

    for (int iteration = 0; iteration < 1; iteration++) {
        i = 0;
        while (i < last_i) {
            container = array[i];
            container = array[i + stride];
            if (i < part_last_i) {
                container = array[i + 2 * stride];
                container = array[i + 3 * stride];
            }
            std::cout << i << " iteration: [" << i << ", " << i + 3 * stride << ")" << std::endl;
            i += 4 * stride;
        }
    }

    delete [] array;

    return 0;
}
