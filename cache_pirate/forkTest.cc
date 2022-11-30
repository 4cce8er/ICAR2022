#include "chp_perf.h"
#include <sched.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    volatile int test_var = 0;
    std::string result_filename = "output.txt";
    const std::string events = "0x5301c0,0x53003c"; // TODO

    // std::string event_configs = "0x5301c0, 0x53003c";
    struct perf_struct *perf = init_perf(events);
    reset_counter(perf);
    start_counter(perf);

    pid_t childPid = fork();

    std::cout << "pid = " << childPid << std::endl;
    if (childPid == 0) { // Child is the target

        const char *my_argv[64] = {"../microbenchmark/icar", NULL};
        
        cpu_set_t my_set;
        CPU_ZERO(&my_set);   /* Initialize it all to 0, i.e. no CPUs selected. */
        CPU_SET(0, &my_set); /* set the bit that represents core 0. */
        int result = sched_setaffinity(getpid(), sizeof(cpu_set_t),
                                       &my_set); // Set affinity of tihs process to  the defined mask, i.e. only 0
        std::cout << "Child pid = " << getpid() << ", cpu = " << CPU_COUNT(&my_set)
                  << ", sched_getcpu() = " << sched_getcpu() << std::endl;

        if (-1 == execve(my_argv[0], (char **)my_argv, NULL)) {
            perror("child process execve failed [%m]");
            return -1;
        }

        // system("../microbenchmark/icar");

        std::cout << "Call completed\n";
        exit(1);

    } else {
        cpu_set_t my_set;
        int result = sched_getaffinity(getpid(), sizeof(my_set), &my_set);
        std::cout << "Parent pid = " << getpid() << ", cpu = " << CPU_COUNT(&my_set)
                  << ", sched_getcpu() = " << sched_getcpu() << std::endl;
        CPU_CLR(0, &my_set); /* remove core CPU zero form parent/pirate process */
        result = sched_setaffinity(getpid(), sizeof(cpu_set_t),
                                   &my_set); // Set affinity of tihs process to  the defined mask, i.e. only 0
        std::cout << "Parent pid = " << getpid() << ", cpu = " << CPU_COUNT(&my_set)
                  << ", sched_getcpu() = " << sched_getcpu() << std::endl;

        std::cout << "Waiting the child...\n";
        int status;
        waitpid(childPid, &status, WUNTRACED);
        std::cout << "Child returned\n";
    }

    char *thp_buf;
    size_t buf_size = 2 * 1024 * 1024 * 16;
    thp_buf = (char *)malloc(buf_size);

    for (int i = 0; i < buf_size; i++) {
        thp_buf[i] = test_var++;
    }

    stop_counter(perf);
    if (childPid > 0) {
        read_counter(perf, NULL, result_filename);
    }

    return 0;
}
