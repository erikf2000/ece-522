#include "motor.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define NUM_MOTORS 4

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

void setup(double speed, int acceleration, char *dir) {
  start_GPIO_connection();
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    setup_motor(motor, speed, acceleration, dir);
  }
}

void turn(bool left, double speed) {
  int offset = (int)left;
  for (int motor = offset; motor < NUM_MOTORS; motor += 2) {
    motor_speed(motor, -speed);
  }
}

void circle(bool left, double speed) {
  int offset = (int)left;
  for (int motor = offset; motor < NUM_MOTORS; motor += 2) {
    motor_speed(motor, speed / 2);
  }
}

void reverse(double speed) {
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    motor_speed(motor, -speed);
  }
}

void enable_motors() {

  enableRawMode();
  char input;
  while (true) {
    input = getchar();
    if (input == 'a') {
      printf("Motor starting \n");
      for (int motor = 0; motor < NUM_MOTORS; motor++) {
        start_motor(motor);
      }
    }
    if (input == 's') {
      printf("Motor stopping \n");
      for (int motor = 0; motor < NUM_MOTORS; motor++) {
        stop_motor(motor);
      }
    }
    if (isdigit(input)) {
      printf("Setting speed to %c0%%! \n", input);
      for (int motor = 0; motor < NUM_MOTORS; motor++) {
        motor_speed(motor, (double)(atof(&input) * 10));
      }
    }
    if (input == 'd') {
      printf("Closing program, goodbye! \n");
      break;
    }
  }
  disableRawMode();
}

void cleanup() { shutdown_GPIO_connection(); }