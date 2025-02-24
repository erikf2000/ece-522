#include <pigpio.h>
#include <stdio.h>
#include <unistd.h>

#define SPI_CHANNEL 1 // Equivalent to spi.open(0,1) in Python
#define SPI_BAUD 300000

// GPIO Pin Definitions
#define PP_FRAME 25
#define PP_INT 22
#define PP_ACK 23
#define PP_SW 24

int spiHandle;

void setup() {
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

// Equivalent to ppCMD1()
void ppCMD1(int addr, int cmd, int param1, int param2) {
  unsigned char arg[4] = {addr, cmd, param1, param2};
  unsigned char resp[4] = {0}; // Unused, but could store response

  gpioWrite(PP_FRAME, 1); // FRAME high
  printf("Sending args: %d %d %d %d\n", arg[0], arg[1], arg[2], arg[3]);

  spiXfer(spiHandle, (char *)arg, (char *)resp, 4); // 4-byte SPI transfer

  gpioWrite(PP_FRAME, 0); // FRAME low
  usleep(3000);           // Delay (0.0003 sec)
}

int main() {
  setup();

  if (spiHandle >= 0) {
    ppCMD1(16, 0, 0, 0);
    ppCMD1(16, 48, 1, 64); // setup
    ppCMD1(16, 58, 0, 66); // setup
    ppCMD1(16, 49, 0, 0);  // start
    sleep(5);
    ppCMD1(16, 50, 0, 0); // stop
    spiClose(spiHandle);
  }

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
