#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int running = 1;

void *receive_messages(void *arg) {
    int socket_fd = *((int *)arg);
    char buffer[BUFFER_SIZE];
    	while (running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            printf("\n[system] Disconnected from The Wired.\n");
            running = 0;
            break;
        }
        printf("%s", buffer);
        fflush(stdout);
    }

    return NULL;
}
int main() {
    int socket_fd;
    struct sockaddr_in server_addr;
    pthread_t recv_thread;
    char buffer[BUFFER_SIZE];

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0) {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(socket_fd);
        return 1;
    }

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(socket_fd);
        return 1;
    }
    pthread_create(&recv_thread, NULL, receive_messages, &socket_fd);
    while (running) {
        memset(buffer, 0, sizeof(buffer));

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

if (!running) {
            break;
        }

        send(socket_fd, buffer, strlen(buffer), 0);
        buffer[strcspn(buffer, "\r\n")] = '\0';

if (strcmp(buffer, "/exit") == 0) {
            running = 0;
            break;
        }
    }
	    close(socket_fd);
    pthread_cancel(recv_thread);
    pthread_join(recv_thread, NULL);
   return 0;
}
