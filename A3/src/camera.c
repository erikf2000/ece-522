#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 128
#define BOUNDARY "frame"

FILE *stream = NULL;
int server_sock;

void handle_sigterm(int signum) {
  printf("Process terminated. Cleaning up...\n");
  if (stream) {
    pclose(stream);
    close(server_sock);
    printf("Closing video stream, goodbye!");
  }
}

void send_video(char *client_ip) {
  server_sock = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
  if (server_sock < 0) {
    printf("Error creating socket \n");
    exit(EXIT_FAILURE);
  }

  printf("Socket created \n");
  struct sockaddr_in client_addr;
  socklen_t addr_size = sizeof(client_addr);

  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(PORT);
  inet_pton(client_addr.sin_family, client_ip, &client_addr.sin_addr);

  stream = popen("libcamera-vid -t 0 --inline -n -o -", "r");
  if (!stream) {
    perror("Failed to open video stream");
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  size_t bytes_read;

  printf("Sending camera data... \n");
  while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, stream)) > 0) {
    if (bytes_read > 0)
      sendto(server_sock, buffer, bytes_read, 0,
             (struct sockaddr *)&client_addr, addr_size);
  }

  pclose(stream);
  close(server_sock);
  printf("Closing, goodbye! \n");
}