import time                                  #import the time module
import piplates.MOTORplate as MOTOR          #import the MOTORplate module
print("CONFIG")
MOTOR.dcCONFIG(0,1,'ccw',50.0,1)           #configure dc motor 2 on the MOTORplate at address 0 being configured for clockwise 
                                             #motion at a 50% duty cycle and 2.5 seconds of acceleration 
print("START")
MOTOR.dcSTART(0,1)                           #Start DC motor
time.sleep(5.0)                              #delay 5 seconds
print("SPEED")
MOTOR.dcSPEED(0,1,100.0)                     #increase speed to 100%
time.sleep(10)                               #wait 10 seconds
print("STOP")
MOTOR.dcSTOP(0,1)                            #stop the motor
time.sleep(2.5)                              #wait for deceleration
print("DC Motor demo completed")            #print notice