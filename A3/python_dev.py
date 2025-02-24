import piplates.MOTORplate as MOTOR
import spidev
import time

# Create an SPI connection
spi = spidev.SpiDev()
spi.open(0, 0)  # Bus 0, Device 0
spi.max_speed_hz = 500000  # Match this in C


def log_spi_transaction(command):
    print(f"Sent: {command}")
    response = spi.xfer2(command)
    print(f"Sent: {command}, Received: {response}")

# Start Motor 1 at 50% speed using Pi-Plates module

args =  [16, 48, 17, 64]

# MOTOR.dcCONFIG(0, 1, 'cw', 50, 1)  # Configure motor
# MOTOR.dcSTART(0, 1)  # Start motor

c = args
print("c  = ", c)
log_spi_transaction(c)  # Example structure

time.sleep(3)                               #wait 10 seconds
MOTOR.dcSTOP(0,1)                            

spi.close()