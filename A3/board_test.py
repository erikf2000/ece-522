import time                                  #import the time module
import piplates.MOTORplate as MOTOR          #import the MOTORplate module
# print("CONFIG")
# MOTOR.dcCONFIG(0,1,'ccw',50.0,1)           #configure dc motor 2 on the MOTORplate at address 0 being configured for clockwise 
#                                              #motion at a 50% duty cycle and 2.5 seconds of acceleration 
# print("START")
# MOTOR.dcSTART(0,1)                           #Start DC motor
# time.sleep(5.0)                              #delay 5 seconds
# print("SPEED")
# MOTOR.dcSPEED(0,1,100.0)                     #increase speed to 100%
# time.sleep(10)                               #wait 10 seconds
# print("STOP")
# MOTOR.dcSTOP(0,1)                            #stop the motor
# time.sleep(2.5)                              #wait for deceleration
# print("DC Motor demo completed")            #print notice

def run():
    try:
        MOTOR.dcCONFIG(0,1,'ccw',50.0,1)           #configure dc motor 2 on the MOTORplate at address 0 being configured for clockwise 
        while True:
            key = input().strip()  # Read user input
            
            if key == 'a':
                print("Starting motor")
                MOTOR.dcSTART(0,1)                           #Start DC motor
            if key == 's':
                print("Stopping motor")
                MOTOR.dcSTOP(0, 1)
            if key in ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']:
                print(f"increasing speed to {key}0%")
                MOTOR.dcSPEED(0, 1, int(key) * 10)
            if key == 'f':
                print("increase speed!")
                MOTOR.dcSPEED(0,1,100.0)                     #increase speed to 100%
            if key == 'd':
                print("Closing, goodbye!")
                break
    except KeyboardInterrupt:
        pass




if __name__ == "__main__":
    run()
