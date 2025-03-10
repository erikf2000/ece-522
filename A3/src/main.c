#include "camera.h"
#include "config.h"
#include "control.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  pid_t pid;
  pid = fork();

  if (pid < 0) {
    perror("Fork failed");
    exit(EXIT_FAILURE);
  } else if (pid) { // parent
    send_video(CLIENT_IP);
    pause();
    int status;
    waitpid(pid, &status, 0);
  } else { // child
    setup(.5, 0, "w");
    enable_motors();
    cleanup();

    printf("Rover shutting down, terminating program...\n");
    kill(getppid(), SIGTERM);
    exit(EXIT_SUCCESS);
  }
  return 0;
}