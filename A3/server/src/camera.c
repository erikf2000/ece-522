#include "config.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 128
#define BOUNDARY "frame"

FILE *stream = NULL;
int video_sock;
volatile sig_atomic_t take_photo_flag = 0;

void handle_signal(int signum) {
  switch (signum) {
  case SIGINT:
    printf("Process terminated. Cleaning up...\n");
    if (stream) {
      pclose(stream);
      close(video_sock);
      printf("Closing video stream, goodbye!");
    }
    exit(EXIT_SUCCESS);
  case SIGUSR1:
    take_photo_flag = 1;
    return;
  }
}

void take_photo() {
  system("libcamera-still --autofocus-on-capture --vflip --hflip --nopreview "
         "-o " PHOTO_PATH);
  printf("Photo taken and saved to %s\n", PHOTO_PATH);
}

void send_photo() {
  int server_sock, client_sock;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_size = sizeof(client_addr);
  FILE *photo_file;
  char buffer[BUFFER_SIZE];
  size_t bytes_read;

  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  // allows multiple photos to be taken quickly, reusing the same address
  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    perror("Setsockopt failed");
    close(server_sock);
    exit(EXIT_FAILURE);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PHOTO_PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Bind failed");
    close(server_sock);
    exit(EXIT_FAILURE);
  }
  if (listen(server_sock, 1) < 0) {
    perror("Listen failed");
    close(server_sock);
    exit(EXIT_FAILURE);
  }
  printf("Waiting for photo request...\n");
  client_sock =
      accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
  if (client_sock < 0) {
    perror("Accept failed");
    close(server_sock);
    exit(EXIT_FAILURE);
  }
  printf("Client connected. Taking photo...\n");
  // Take the photo when client successfully connects
  take_photo();
  photo_file = fopen(PHOTO_PATH, "rb");
  if (!photo_file) {
    perror("Failed to open photo file");
    close(client_sock);
    close(server_sock);
    return;
  }
  // Send the photo to the client
  while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, photo_file)) > 0) {
    send(client_sock, buffer, bytes_read, 0);
  }

  fclose(photo_file);
  close(client_sock);
  close(server_sock);
  remove(PHOTO_PATH);
  printf("Photo sent to client successfully\n");
}

void send_camera_data(char *client_ip) {
  struct sigaction sa;
  sa.sa_handler = handle_signal;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGUSR1, &sa, NULL);

  video_sock = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket for video
  if (video_sock < 0) {
    printf("Error creating socket \n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in client_addr;
  socklen_t addr_size = sizeof(client_addr);

  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(VIDEO_PORT);
  inet_pton(client_addr.sin_family, client_ip, &client_addr.sin_addr);

  stream = popen("libcamera-vid -t 0 --inline --vflip --hflip -n --width 1920 "
                 "--height 1080 -o -",
                 "r");
  if (!stream) {
    perror("Failed to open video stream");
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  size_t bytes_read;

  printf("Sending camera data... \n");
  while (true) {
    if (take_photo_flag) {
      take_photo_flag = 0;
      pclose(stream);
      send_photo();
      stream = popen("libcamera-vid -t 0 --inline --vflip --hflip -n --width "
                     "1920 --height 1080 -o -",
                     "r");
      sleep(1);
    }
    bytes_read = fread(buffer, 1, BUFFER_SIZE, stream);
    if (bytes_read > 0) {
      sendto(video_sock, buffer, bytes_read, 0, (struct sockaddr *)&client_addr,
             addr_size);
    }
  }
  pclose(stream);
  close(video_sock);
  printf("Closing, goodbye! \n");
}
