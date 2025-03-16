#include "config.h"
#include "motor.h"
#include <arpa/inet.h>
#include <ctype.h>
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

struct timespec ts = {.tv_sec = 0, .tv_nsec = 10000};
struct timespec ts2 = {.tv_sec = 0, .tv_nsec = 300000000};

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

void off() {
  printf("stopping motors \n");
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    stop_motor(motor);
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

void forward() {
  printf("starting motors \n");
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    setup_motor(motor, motors[motor].speed, 0, motors[motor].reverse);
    motor_speed(motor, motors[motor].speed);
    start_motor(motor);
  }
}

void reverse() {
  printf("reversing! \n");
  for (int motor = 0; motor < NUM_MOTORS; motor++) {
    setup_motor(motor, motors[motor].speed, 0, !motors[motor].reverse);
    motor_speed(motor, motors[motor].speed);
    start_motor(motor);
  }
}

int kbhit() {
  struct timeval tv = {0L, 0L};
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
  // return FD_ISSET(STDIN_FILENO, &fds);
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
  }
}

bool process_input(bool *key_pressed) {
  bool is_click = false;
  while (kbhit()) {
    is_click = true;
    unsigned char key = (unsigned char)getchar();
    printf("keystroke = %c \n", key);
    if (key == 'q') {
      memset(key_pressed, false, sizeof(bool) * 256);
      return false;
    }
    if (key_pressed[key]) {
      return true;
    }
    key_pressed[key] = true;
    nanosleep(&ts, NULL);
  }
  if (!is_click) {
    memset(key_pressed, false, sizeof(bool) * 256);
  }
  return true;
}

int get_keypress() {
  struct termios oldt, newt;
  int ch;
  struct timeval tv = {0, 1000}; // Small delay to avoid 100% CPU usage
  fd_set set;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  FD_ZERO(&set);
  FD_SET(STDIN_FILENO, &set);
  if (select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) > 0) {
    ch = getchar();
  } else {
    ch = 0;
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}

void receive_commands() {
  int server_sock, client_sock;
  server_sock = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
  if (server_sock < 0) {
    printf("Error creating socket \n");
    exit(EXIT_FAILURE);
  }

  printf("Socket created \n");
  struct sockaddr_in server_addr;
  socklen_t addr_size = sizeof(server_addr);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(CONTROL_PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_sock, (const struct sockaddr *)&server_addr, addr_size) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(server_sock, 1) < 0) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d..\n", CONTROL_PORT);

  if ((client_sock = accept(server_sock, (struct sockaddr *)&server_addr,
                            &addr_size)) < 0) {
    perror("Socket failed");
    exit(EXIT_FAILURE);
  }
  int state = MOTOR_OFF;

  while (true) {
    int bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
    if (bytes_read) {
      printf("Button Pressed! %s \n", buffer);
      unsigned char key = buffer[0];
      if (key == 'q') {
        break;
      }
      if (key == 'u') {
        key = buffer[1];
        key_pressed[key] = false;
        printf("key %c released \n", key);
      } else {
        key_pressed[key] = true;
        printf("key %c pressed \n", key);
      }
      int next_state = set_state(key_pressed);
      printf("states, new and old: %d, %d \n", next_state, state);
      if (next_state != state) {
        state = next_state;
        set_motors(state);
      }
      memset(buffer, 0, sizeof(buffer));
    }
  }
  close(client_sock);
  close(server_sock);
}

void enable_motors() {

  receive_commands();

  // enableRawMode();
  // int i = 0;
  // char c;
  // bool down = false;
  // while (!i) {
  // usleep(1);
  // if (kbhit()) {
  //   while (kbhit()) {
  //     c = getchar();

  //     if (c == 'q')
  //       i = 1;
  //     else
  //       i = 0;
  //     if (!down) {
  //       down = true;
  //       printf("key down %c \n", c);
  //     }
  //   }
  //   usleep(20000);
  //   if ((c = get_keypress())) {
  //     if (c == 'q')
  //       i = 1;
  //     else
  //       i = 0;
  //     if (!down) {
  //       down = true;
  //       printf("key down %c \n", c);
  //     }
  //   } else {
  //     if (down) {
  //       down = false;
  //       printf("key up \n");
  //     }
  //   }
  //   usleep(160000);
  // }
  // printf("you hit %c. \n", c);
  // disableRawMode();
  // return;

  // int prev_state = MOTOR_OFF;
  // bool inputs[256] = {false};
  // while (process_input(inputs)) {
  //   int next_state = set_state(inputs);
  //   printf("state = %d \n", next_state);
  //   if (prev_state != next_state) {
  //     printf("previous states are different!\n");
  //     set_motors(next_state);
  //     prev_state = next_state;
  //   } else {
  //     printf("previous states are the same\n");
  //   }

  //   nanosleep(&ts2, NULL);

  //   if (input == 'a') {
  //     printf("Motor starting \n");
  //     for (int motor = 0; motor < NUM_MOTORS; motor++) {
  //       start_motor(motor);
  //     }
  //   }
  //   if (input == 's') {
  //     printf("Motor stopping \n");
  //     for (int motor = 0; motor < NUM_MOTORS; motor++) {
  //       stop_motor(motor);
  //     }
  //   }
  //   if (input == 'r') {
  //     printf("Motors reversing: \n");
  //     reverse(motors[0].speed);
  //   }
  //   if (isdigit(input)) {
  //     printf("Setting speed to %c0%%! \n", input);
  //     for (int motor = 0; motor < NUM_MOTORS; motor++) {
  //       double speed = (double)(atof(&input) * 10);
  //       motors[motor].speed = speed;
  //       motor_speed(motor, speed);
  //     }
  //   }
  //   if (input == 'c') {
  //     printf("Circling: \n");
  //     circle(true);
  //   }
  //   if (input == 'v') {
  //     printf("Circling (right):");
  //     circle(false);
  //   }
  //   if (input == 'd') {
  //     printf("Closing program, goodbye! \n");
  //     break;
  //   }
  // }
  printf("Quitting program, goodbye!\n");
  disableRawMode();
}

void cleanup() { shutdown_GPIO_connection(); }