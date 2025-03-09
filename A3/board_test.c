#include <ctype.h>
#include <fcntl.h>
#include <pigpio.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define SPI_CHANNEL 1 // Equivalent to spi.open(0,1) in Python
#define SPI_BAUD 300000

// GPIO Pin Definitions
#define PP_FRAME 25
#define PP_INT 22
#define PP_ACK 23
#define PP_SW 24

int spiHandle;

struct timespec ts;

void ppCMD1(int addr, int cmd, int param1, int param2) {
  unsigned char arg[4] = {addr, cmd, param1, param2};
  unsigned char resp[4] = {0}; // Unused, but could store response

  gpioWrite(PP_FRAME, 1); // FRAME high
  printf("Sending args: %d %d %d %d\n", arg[0], arg[1], arg[2], arg[3]);

  spiXfer(spiHandle, (char *)arg, (char *)resp, 4); // 4-byte SPI transfer
  gpioDelay(40);

  gpioWrite(PP_FRAME, 0); // FRAME low
  nanosleep(&ts, NULL);   // Delay (0.0003 sec)
}

void preamble(int motor, double speed, int acceleration, char *dir) {
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

  // speed = int((speed*1023/100)+0.5)
  // if ((motor==1)or(motor==2)):
  //     speed=(speed*5)>>3
  // // Param1:|motor num 1|motor num 0|NA|direction|NA|NA|dc9|dc8|
  // param1=(motor-1)<<6
  // if dir=='cw':
  //     param1=param1+0x10
  // param1=param1+(speed>>8)
  // param2= speed & 0x00FF
  // ppCMDm(addr,0x30,param1,param2,0)
}

void start(int motor) {
  ppCMD1(16, 49, 0, 0); // start
}

void stop(int motor) {
  ppCMD1(16, 50, 0, 0); // stop
}

void speed(int motor, double speed) {
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

void setup() {
  if (gpioInitialise() < 0) {
    printf("Failed to initialize pigpio\n");
    return;
  }
  ts.tv_sec = 0;
  ts.tv_nsec = 300000;

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
  } else {
    // ppCMD1(16, 0, 0, 0);
    ppCMD1(16, 48, 1, 64); // setup
    ppCMD1(16, 58, 0, 66); // setup
  }
}

void enableRawMode() {
  struct termios tty;
  tcgetattr(STDIN_FILENO, &tty);          // Get current terminal attributes
  tty.c_lflag &= ~(ICANON | ECHO);        // Disable canonical mode & echo
  tcsetattr(STDIN_FILENO, TCSANOW, &tty); // Apply changes
}

void disableRawMode() {
  struct termios tty;
  tcgetattr(STDIN_FILENO, &tty);
  tty.c_lflag |= (ICANON | ECHO); // Restore settings
  tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void run() {

  enableRawMode();
  char input;
  while (true) {
    input = getchar();
    if (input == 'a') {
      printf("Motor starting \n");
      start(0);
    }
    if (input == 's') {
      printf("Motor stopping \n");
      stop(0);
    }
    if (isdigit(input)) {
      printf("Setting speed to %c0%%! \n", input);
      speed(0, (double)((input - 0) * 10));
    }
    if (input == 'd') {
      printf("Closing program, goodbye! \n");
      break;
    }
  }
  disableRawMode();
}

int main() {
  setup();
  run();
  spiClose(spiHandle);
  gpioTerminate();
  return 0;
}

// int main() {

//   if (gpioInitialise() < 0) {
//     printf("pigpio initialization failed.\n");
//     return -1;
//   }

//   int spi_handle = spiOpen(SPI_CHANNEL, SPI_SPEED, SPI_BITS);

//   if (spi_handle < 0) {
//     printf("Failed to open SPI device.\n");
//     gpioTerminate();
//     return -1;
//   }

//   char tx_buffer[4];       // Transmit buffer
//   char rx_buffer[4] = {0}; // Clear receive buffer

//   tx_buffer[0] = 16; // Device Address (0 for first Pi-Plate)
//   tx_buffer[1] = 0;  // Command: ???
//   tx_buffer[2] = 0;  // Motor 1
//   tx_buffer[3] = 0;  // ???

//   spiXfer(spi_handle, tx_buffer, rx_buffer, sizeof(tx_buffer));
//   gpioDelay(40);
//   printf("IDK Received: 0x%X 0x%X 0x%X 0x%X\n", rx_buffer[0], rx_buffer[1],
//          rx_buffer[3], rx_buffer[4]);

//   // Start Motor 1 at 50% speed
//   tx_buffer[1] = 48; // Command: Config DC Motor
//   tx_buffer[2] = 1;  // ??
//   tx_buffer[3] = 64; // speed

//   spiXfer(spi_handle, tx_buffer, rx_buffer, sizeof(tx_buffer));
//   gpioDelay(40);
//   printf("Setup Received: 0x%X 0x%X 0x%X 0x%X\n", rx_buffer[0], rx_buffer[1],
//          rx_buffer[3], rx_buffer[4]);

//   sleep(1);

//   tx_buffer[1] = 58; // Command: Config DC Motor
//   tx_buffer[2] = 0;  // ??
//   tx_buffer[3] = 66; // acceleration

//   spiXfer(spi_handle, tx_buffer, rx_buffer, sizeof(tx_buffer));
//   gpioDelay(40);
//   printf("Setup Received: 0x%X 0x%X 0x%X 0x%X\n", rx_buffer[0], rx_buffer[1],
//          rx_buffer[3], rx_buffer[4]);

//   sleep(1);

//   tx_buffer[1] = 49; // Command: Start DC Motor
//   tx_buffer[2] = 0;  // Motor 1
//   tx_buffer[3] = 0;  // ??

//   spiXfer(spi_handle, tx_buffer, rx_buffer, sizeof(tx_buffer));
//   gpioDelay(40);
//   printf("Start Received: 0x%X 0x%X 0x%X 0x%X\n", rx_buffer[0], rx_buffer[1],
//          rx_buffer[3], rx_buffer[4]);

//   sleep(5); // Wait for 5 seconds

//   // Stop Motor 1
//   tx_buffer[1] = 50; // Command: Stop DC Motor
//   tx_buffer[2] = 0;  // Motor 1
//   tx_buffer[3] = 0;  // ??

//   spiXfer(spi_handle, tx_buffer, rx_buffer, sizeof(tx_buffer));
//   gpioDelay(40);
//   printf("Stop Received: 0x%X 0x%X 0x%X 0x%X\n", rx_buffer[0], rx_buffer[1],
//          rx_buffer[3], rx_buffer[4]);

//   spiClose(spi_handle);

//   // Terminate pigpio library
//   gpioTerminate();

//   return 0;
// }
