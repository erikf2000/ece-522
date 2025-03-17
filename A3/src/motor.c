#include <ctype.h>
#include <fcntl.h>
#include <pigpio.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SPI_CHANNEL 1
#define SPI_BAUD 300000

// GPIO Pin Definitions
#define PP_FRAME 25
#define PP_INT 22
#define PP_ACK 23
#define PP_SW 24
#define RMAX 2000

int spiHandle;

struct timespec ts_0003 = {.tv_sec = 0, .tv_nsec = 300000};
struct timespec ts_001 = {.tv_sec = 0, .tv_nsec = 1000000};

void ppCMD1(int addr, int cmd, int param1, int param2) {
  unsigned char arg[4] = {addr, cmd, param1, param2};
  unsigned char resp[4] = {0};

  gpioWrite(PP_FRAME, 1); // FRAME high

  spiXfer(spiHandle, (char *)arg, (char *)resp, 4); // 4-byte SPI transfer
  gpioDelay(40);

  gpioWrite(PP_FRAME, 0);    // FRAME low
  nanosleep(&ts_0003, NULL); // Delay (0.0003 sec)
}

void setup_motor(int motor, double speed, int acceleration, bool reverse) {

  int param1;
  int param2;

  int motorSpeed = (int)((speed * 1023 / 100) + 0.5);
  if (motor == 0 || motor == 1) {
    motorSpeed = (motorSpeed * 5) >> 3;
  }

  param1 = motor << 6;
  if (!reverse) {
    param1 = param1 + 0x10;
  }
  param1 += (motorSpeed >> 8);
  param2 = motorSpeed & 0x00FF;
  ppCMD1(16, 0x30, param1, param2);
  nanosleep(&ts_001, NULL); // Delay (0.001 sec)

  int increment = 0;

  if (acceleration != 0) {
    increment = (int)1024 * motorSpeed / (acceleration * RMAX) + 0.5;
  }
  param1 = (increment >> 8);
  param2 = increment & 0x00FF;

  ppCMD1(16, 0x3A + motor, param1, param2);
}

void start_motor(int motor) {
  ppCMD1(16, 49, motor, 0); // start
}

void stop_motor(int motor) {
  ppCMD1(16, 50, motor, 0); // stop
}

void motor_speed(int motor, double speed) {
  int param1;
  int param2;
  int motorSpeed = (int)((speed * 1023 / 100) + 0.5);
  if (motor == 0 || motor == 1) {
    motorSpeed = (motorSpeed * 5) >> 3;
  }
  param1 = motor << 6;
  param1 += (motorSpeed >> 8);
  param2 = motorSpeed & 0x00FF;
  ppCMD1(16, 0x33, param1, param2);
  nanosleep(&ts_001, NULL); // Delay .001 sec
}

void start_GPIO_connection() {
  if (gpioInitialise() < 0) {
    printf("Failed to initialize pigpio\n");
    return;
  }

  // Set up GPIO
  gpioSetMode(PP_FRAME, PI_OUTPUT);
  gpioWrite(PP_FRAME, 0);

  gpioSetMode(PP_INT, PI_INPUT);
  gpioSetPullUpDown(PP_INT, PI_PUD_UP);

  gpioSetMode(PP_ACK, PI_INPUT);
  gpioSetPullUpDown(PP_ACK, PI_PUD_UP);

  gpioSetMode(PP_SW, PI_INPUT);
  gpioSetPullUpDown(PP_SW, PI_PUD_UP);

  spiHandle = spiOpen(SPI_CHANNEL, SPI_BAUD, 0);
  if (spiHandle < 0) {
    printf("Failed to open SPI\n");
  }
}

void shutdown_GPIO_connection() {
  spiClose(spiHandle);
  gpioTerminate();
}