#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define NUM_SWITCHES 100000

int main() {
  int fd_pc[2];
  int fd_cp[2];

  char val = '0';
  struct timespec start, end;

  if (pipe(fd_pc) == -1 || pipe(fd_cp) == -1) {
    exit(EXIT_FAILURE);
  }
  pid_t p = fork();

  if (p < 0) { // error
    exit(EXIT_FAILURE);
  }
  if (p == 0) { // child process
    for (int i = 0; i < NUM_SWITCHES; i++) {
      read(fd_cp[0], &val, sizeof(val));
      write(fd_pc[1], &val, sizeof(val));
    }
    exit(EXIT_SUCCESS);
  } else { // parent process
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < NUM_SWITCHES; i++) {
      write(fd_cp[1], &val, sizeof(val));
      read(fd_pc[0], &val, sizeof(val));
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    // total time (in nanoseconds) of child and parent context switching
    long long total_time = (long long)((end.tv_sec * 1e9 + end.tv_nsec) -
                                       (start.tv_sec * 1e9 + start.tv_nsec));

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 2 * NUM_SWITCHES; i++) {
      // prevent compiler optimization
      asm volatile("nop");
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    // time looping through without context swithcing
    long long loop_time = (long long)((end.tv_sec * 1e9 + end.tv_nsec) -
                                      (start.tv_sec * 1e9 + start.tv_nsec));

    printf("Total time: %.2f seconds \n", (total_time / 1e9));
    printf("Loop time without context switching: %.2f miliseconds\n",
           loop_time / 1e6);
    printf("Average context switch time: %.2f microseconds\n",
           (total_time - loop_time) / (2 * NUM_SWITCHES) / 1e3);
  }
  return 0;
}