#include "config.h"
#include "motor.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
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

char buffer[1024];

bool key_pressed[256];

void setup(double speed, int acceleration) {
  start_GPIO_connection();
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    motors[motor].speed = speed;
    motors[motor].reverse = (motor % 2) ? true : false;
    setup_motor(motor, speed, acceleration, motors[motor].reverse);
  }
}

void off() {
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    stop_motor(motor);
  }
}

void spin(bool right) {
  int offset = (int)right;
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    if (motor % 2 == offset) {
      setup_motor(motor, motors[motor].speed, 0, !motors[motor].reverse);
    } else {
      setup_motor(motor, motors[motor].speed, 0, motors[motor].reverse);
    }
    motor_speed(motor, motors[motor].speed * 2.2);
    start_motor(motor);
  }
}

void circle(bool right) {
  int offset = (int)right;
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    if (motor % 2 == offset) {
      motor_speed(motor, motors[motor].speed / 2);
    } else {
      motor_speed(motor, motors[motor].speed * 2.2);
    }
  }
}

void forward() {
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    setup_motor(motor, motors[motor].speed, 0, motors[motor].reverse);
    motor_speed(motor, motors[motor].speed);
    start_motor(motor);
  }
}

void reverse() {
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    setup_motor(motor, motors[motor].speed, 0, !motors[motor].reverse);
    motor_speed(motor, motors[motor].speed);
    start_motor(motor);
  }
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

void set_motors(int state) {
  switch (state) {
  case MOTOR_OFF:
    off();
    break;
  case MOTOR_FORWARD:
    forward();
    break;
  case MOTOR_CIRCLE_LEFT:
    forward();
    circle(false);
    break;
  case MOTOR_CIRCLE_RIGHT:
    forward();
    circle(true);
    break;
  case MOTOR_REVERSE:
    reverse();
    break;
  case MOTOR_REVERSE_CIRCLE_LEFT:
    reverse();
    circle(false);
    break;
  case MOTOR_REVERSE_CIRCLE_RIGHT:
    reverse();
    circle(true);
    break;
  case MOTOR_SPIN_LEFT:
    spin(false);
    break;
  case MOTOR_SPIN_RIGHT:
    spin(true);
    break;
  }
}

void send_photo_signal(int client_sock) {
  kill(getppid(), SIGUSR1);
  printf("Sent signal to take photo to camera proccess.\n");
}

void receive_commands() {
  int server_sock, client_sock;
  server_sock = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
  if (server_sock < 0) {
    printf("Error creating socket \n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_size = sizeof(client_addr);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(CONTROL_PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_sock, (const struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(server_sock, 1) < 0) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr,
                            &addr_size)) < 0) {
    perror("Socket failed");
    exit(EXIT_FAILURE);
  }
  int state = MOTOR_OFF;

  while (true) {
    int bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
    if (bytes_read) {
      unsigned char key = buffer[0];
      if (key == 'q') {
        break;
      }
      if (key == 'p') {
        send_photo_signal(client_sock);
      }
      if (key == 'u') {
        key = buffer[1];
        key_pressed[key] = false;
      } else {
        key_pressed[key] = true;
      }
      int next_state = set_state(key_pressed);
      if (next_state != state) {
        state = next_state;
        set_motors(state);
      }
      memset(buffer, 0, sizeof(buffer));
    }
  }
  close(client_sock);
  close(server_sock);
  printf("Command loop closing, goodbye!\n");
}

void cleanup() { shutdown_GPIO_connection(); }