#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ITERATIONS 10000000

int main() {
  struct timespec first_time, prev_time, current_time;
  volatile double x = 2.815324, y = 18.93415433, z;
  double current, total;

  clock_gettime(CLOCK_MONOTONIC, &first_time);
  prev_time = first_time;

  for (int i = 0; i < ITERATIONS; i++) { // test addition
    z = x + y;
  }
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  current = ((current_time.tv_sec * 1e9 + current_time.tv_nsec) -
             (prev_time.tv_sec * 1e9 + prev_time.tv_nsec));
  printf("Addition time: %.2lf nanoseconds\n", current / ITERATIONS);

  prev_time = current_time;
  for (int i = 0; i < ITERATIONS; i++) { // test mulitplication
    z = x * y;
  }
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  current = ((current_time.tv_sec * 1e9 + current_time.tv_nsec) -
             (prev_time.tv_sec * 1e9 + prev_time.tv_nsec));
  printf("Multiplication time: %.2lf nanoseconds\n", current / ITERATIONS);

  prev_time = current_time;
  for (int i = 0; i < ITERATIONS; i++) { // test division
    z = x / y;
  }
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  current = ((current_time.tv_sec * 1e9 + current_time.tv_nsec) -
             (prev_time.tv_sec * 1e9 + prev_time.tv_nsec));
  printf("Division time: %.2lf nanoseconds\n", current / ITERATIONS);

  total = ((current_time.tv_sec * 1e9 + current_time.tv_nsec) -
           (first_time.tv_sec * 1e9 + first_time.tv_nsec)) /
          1e6;

  printf("Total time: %.2lf milliseconds\n", total);

  return 0;
}