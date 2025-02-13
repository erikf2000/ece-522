#include <math.h>
#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ITERATIONS 100000

int main() {
  struct timespec prev_time, current_time;
  double max_jitter = 0.0, prev = 0.0, total = 0.0;

  clock_gettime(CLOCK_MONOTONIC, &prev_time);

  for (int i = 0; i < ITERATIONS; i++) {
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    double current = ((current_time.tv_sec * 1e9 + current_time.tv_nsec) -
                      (prev_time.tv_sec * 1e9 + prev_time.tv_nsec)) /
                     1e3;

    // jitter is the absolute difference between current time and previous
    double jitter = fabs(current - prev);
    if (prev) { // compare to max jitter and set if higher
      max_jitter = fmax(jitter, max_jitter);
    }
    total += jitter;
    prev = current;
  }

  printf("Max jitter: %.2f microseconds \n", max_jitter);
  printf("Average jitter: %.2f microseconds \n", total / ITERATIONS);

  return 0;
}