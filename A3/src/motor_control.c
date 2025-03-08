#include <ctype.h>
#include <fcntl.h>
#include <pigpio.h>
#include <stdio.h>
#include <string.h>

#define SPI_CHANNEL 1 // Equivalent to spi.open(0,1) in Python
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
  unsigned char resp[4] = {0}; // Unused, but could store response

  gpioWrite(PP_FRAME, 1); // FRAME high
  printf("Sending args: %d %d %d %d\n", arg[0], arg[1], arg[2], arg[3]);

  spiXfer(spiHandle, (char *)arg, (char *)resp, 4); // 4-byte SPI transfer
  gpioDelay(40);

  gpioWrite(PP_FRAME, 0);    // FRAME low
  nanosleep(&ts_0003, NULL); // Delay (0.0003 sec)
  printf("return = %s \n", resp);
}

void setup_motor(int motor, double speed, int acceleration, char *dir) {
  ppCMD1(16, 48, 1, 64); // setup
  ppCMD1(16, 58, 0, 66); // setup

  int param1;
  int param2;

  int motorSpeed = (int)((speed * 1023 / 100) + 0.5);
  if (motor == 0 || motor == 1) {
    motorSpeed = (motorSpeed * 5) >> 3;
  }

  param1 = motor << 6;
  if (strncmp(dir, "cw", 3) == 0) {
    param1 = param1 + 0x10;
  }
  param1 += (motorSpeed >> 8);
  param2 = motorSpeed & 0x00FF;
  ppCMD1(16, 0x30, param1, param2);
  nanosleep(&ts_001, NULL); // Delay (0.0003 sec) UPDATE TO .001

  int increment = 0;

  if (acceleration != 0) {
    increment = (int)1024 * motorSpeed / (acceleration * RMAX) + 0.5;
  }
  param1 = (increment >> 8);
  param2 = increment & 0x00FF;

  ppCMD1(16, 0x3A + motor, param1, param2);
  nanosleep(&ts_001, NULL); // UPDATE TO .001
}

void start_motor(int motor) {
  ppCMD1(16, 49, 0, 0); // start
}

void stop_motor(int motor) {
  ppCMD1(16, 50, 0, 0); // stop
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

  // speed = int((speed*1023/100)+0.5)
  //   if ((motor==1) or (motor==2)):
  //       speed=(speed*5)>>3
  //   ## Param1:|motor num 1|motor num 0|NA|direction|NA|NA|dc9|dc8|
  //   param1=(motor-1)<<6
  //   param1=param1+(speed>>8)
  //   param2= speed & 0x00FF
  //   ppCMDm(addr,0x33,param1,param2,0)
}

void start_GPIO_connection() {
  if (gpioInitialise() < 0) {
    printf("Failed to initialize pigpio\n");
    return;
  }

  // Set up GPIO
  gpioSetMode(PP_FRAME, PI_OUTPUT);
  gpioWrite(PP_FRAME, 0); // Initialize FRAME signal

  gpioSetMode(PP_INT, PI_INPUT);
  gpioSetPullUpDown(PP_INT, PI_PUD_UP);

  gpioSetMode(PP_ACK, PI_INPUT);
  gpioSetPullUpDown(PP_ACK, PI_PUD_UP);

  gpioSetMode(PP_SW, PI_INPUT);
  gpioSetPullUpDown(PP_SW, PI_PUD_UP);

  // Open SPI channel 1 (same as spidev 0,1)
  spiHandle = spiOpen(SPI_CHANNEL, SPI_BAUD, 0);
  if (spiHandle < 0) {
    printf("Failed to open SPI\n");
  }
}

void shutdown_GPIO_connection() {
  spiClose(spiHandle);
  gpioTerminate();
}