#include "motor.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define NUM_MOTORS 4

#define MOTOR_OFF 0
#define MOTOR_FORWARD 1
#define MOTOR_CIRCLE_RIGHT 2
#define MOTOR_CIRCLE_LEFT 3
#define MOTOR_SPIN_RIGHT 4
#define MOTOR_SPIN_LEFT 5
#define MOTOR_REVERSE 6
#define MOTOR_REVERSE_CIRCLE_RIGHT 7
#define MOTOR_REVERSE_CIRCLE_LEFT 8

struct motor {
  double speed;
  bool reverse;
};

struct motor motors[NUM_MOTORS];

bool key_pressed[256];

struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000};

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
    motors[motor].speed = speed;
    motors[motor].reverse = (motor % 2) ? true : false;
    setup_motor(motor, speed, acceleration, motors[motor].reverse);
  }
}

void spin(bool left) {
  int offset = (int)left;
  for (int motor = offset; motor < NUM_MOTORS; motor += 2) {
    motors[motor].reverse = !motors[motor].reverse;
    motor_speed(motor, motors[motor].speed);
  }
}

void circle(bool left) {
  int offset = (int)left;
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    // motors[motor].speed = speed / 2;
    if (motor % 2 == offset) {
      motor_speed(motor, motors[motor].speed / 2);
    } else {
      motor_speed(motor, motors[motor].speed * 2);
    }
  }
}

void reverse(double speed) {
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    motors[motor].reverse = !motors[motor].reverse;
    setup_motor(motor, motors[motor].speed, 0, motors[motor].reverse);
  }
}

void off() {
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    stop_motor(motor);
  }
}

void forward() {
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    setup_motor(motor, motors[motor].speed, 0, motors[motor].reverse);
    start_motor(motor);
  }
}

// void reverse() {
//   for (int motor = 0; motor < NUM_MOTORS; motor++) {
//     setup_motor(motor, motors[motor].speed, 0, !motors[motor].reverse);
//     start_motor(motor);
//   }
// }

int kbhit() {
  struct timeval tv = {0L, 0L};
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

int set_state(bool *keys) {
  if (keys['w'] && keys['d']) {
    return MOTOR_CIRCLE_RIGHT;
  } else if (keys['w'] && keys['a']) {
    return MOTOR_CIRCLE_LEFT;
  } else if (keys['w']) {
    return MOTOR_FORWARD;
  } else if (keys['s'] && keys['d']) {
    return MOTOR_REVERSE_CIRCLE_RIGHT;
  } else if (keys['s'] && keys['a']) {
    return MOTOR_REVERSE_CIRCLE_LEFT;
  } else if (keys['s']) {
    return MOTOR_REVERSE;
  } else if (keys['d']) {
    return MOTOR_SPIN_RIGHT;
  } else if (keys['a']) {
    return MOTOR_SPIN_LEFT;
  }
  return MOTOR_OFF;
}

// void set_motors(int state) {
//   switch (state) {
//   case MOTOR_OFF:
//     off();
//     break;
//   case MOTOR_FORWARD:
//     forward();
//     break;
//   case MOTOR_CIRCLE_LEFT:
//     forward();
//     circle(true);
//     break;
//   case MOTOR_CIRCLE_RIGHT:
//     forward();
//     circle(false);
//     break;
//   case MOTOR_REVERSE:
//     reverse();
//   case MOTOR_REVERSE_CIRCLE_LEFT:
//     reverse();
//     circle(true);
//   case MOTOR_REVERSE_CIRCLE_RIGHT:
//     reverse();
//     circle(false);
//   }
// }

// void process_input(bool *key_pressed) {
//   bool is_click = false;
//   while (kbhit()) {
//     is_click = true;
//     nanosleep(&ts, NULL);
//     unsigned char key = (unsigned char)getchar();
//     if (key_pressed[key]) {
//       return;
//     }
//     key_pressed[key] = true;
//   }
//   if (!is_click) {
//     printf("no keystroke :( \n");
//     memset(key_pressed, false, sizeof(bool) * 256);
//   }
// }

void enable_motors() {

  enableRawMode();
  // int chars = 256;
  // int prev_state = MOTOR_OFF;
  // bool inputs[256] = {false};
  char input;
  while (true) {
    input = getchar();
    // process_input(inputs);
    // int next_state = set_state(inputs);
    // printf("state = %d", next_state);
    // if (prev_state != next_state) {
    // set_motors(next_state);
    //   next_state = prev_state;
    //  }

    // nanosleep(&ts, NULL);

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
    if (input == 'r') {
      printf("Motors reversing: \n");
      reverse(motors[0].speed);
    }
    if (isdigit(input)) {
      printf("Setting speed to %c0%%! \n", input);
      for (int motor = 0; motor < NUM_MOTORS; motor++) {
        double speed = (double)(atof(&input) * 10);
        motors[motor].speed = speed;
        motor_speed(motor, speed);
      }
    }
    if (input == 'c') {
      printf("Circling: \n");
      circle(true);
    }
    if (input == 'v') {
      printf("Circling (right):");
      circle(false);
    }
    if (input == 'd') {
      printf("Closing program, goodbye! \n");
      break;
    }
  }
  disableRawMode();
}

void cleanup() { shutdown_GPIO_connection(); }