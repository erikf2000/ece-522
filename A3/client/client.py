import pygame
import socket
import subprocess
import time
from config import *

pygame.init()
WIDTH, HEIGHT = 640, 480 
BUFFER_SIZE = 4096
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Rover Controller with Camera Feed")

# Control Socket (TCP)
control_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
control_socket.connect((CONTROL_IP, CONTROL_PORT))

# Video Stream (UDP)
ffplay_cmd = ["ffplay", VIDEO_STREAM_URL, "-an", "-x", str(WIDTH), "-y", str(HEIGHT), 
              "-fflags", "nobuffer", "-flags", "low_delay", "-framedrop"]
ffplay_process = subprocess.Popen(ffplay_cmd)

# Movement mapping
key_to_command = {
    pygame.K_w: "w",
    pygame.K_s: "s",
    pygame.K_a: "a",
    pygame.K_d: "d",
    pygame.K_q: "q",
    pygame.K_p: "p",
}

running = True
# Input Handling (Main Thread)
while running:
    for event in pygame.event.get():
        if event.type == pygame.KEYDOWN:
            if event.key in key_to_command:
                command = key_to_command[event.key]
                control_socket.sendall(command.encode())  # Send command to Pi
                print(f"Sent: {command}")
                if command == 'q':
                    running = False
                if command == 'p':
                    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as photo_sock:
                        time.sleep(1)
                        photo_sock.connect((CONTROL_IP, PHOTO_PORT))
                        timestamp = time.strftime("%Y-%m-%d_%H-%M-%S", time.localtime())
                        filename = f"rover_capture_{timestamp}.jpg"
                        # Receive the photo data in chunks
                        with open(filename, "wb") as photo_file:
                            while True:
                                data = photo_sock.recv(4096)
                                if not data:
                                    break
                                photo_file.write(data)
                        print("Photo received successfully")
                        photo_sock.close()


        elif event.type == pygame.KEYUP:
            if event.key in key_to_command:
                command = key_to_command[event.key]
                control_socket.sendall(('u' + command).encode())  # Stop when key is released
                print("Sent: stop")
    

pygame.quit()
control_socket.close()
ffplay_process.terminate()
