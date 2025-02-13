#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define PIN 17

volatile uint32_t start_time = 0;
volatile uint32_t end_time = 0;

void callback(int gpio, int level, uint32_t tick) {
  if (level == 1) { // interrupt activated
    end_time = tick;
    uint32_t total_time = (end_time - start_time);
    printf("Inturrupt latency: %u \n", total_time);
  }
}

int main() {
  if (gpioInitialise() < 0) { // initialize GPIO
    printf("Gpio initialization failed \n");
    return -1;
  }

  gpioSetMode(PIN, PI_INPUT);
  gpioSetAlertFunc(PIN, callback);

  start_time = gpioTick();

  gpioWrite(PIN, 1);
  gpioDelay(1000);
  gpioWrite(PIN, 0);

  gpioTerminate();

  printf("Success! \n");
  return 0;
}