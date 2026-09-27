#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "iot_server.h"

static volatile sig_atomic_t keep_running = 1;

static void sigint_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

static void handle_new_connection(int listen_fd, client_t *clients) {
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int new_socket = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);

    if (new_socket < 0) {
        if (errno != EINTR && errno != EAGAIN) {
            perror("[Server] accept failed");
        }
        return;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1) {
            clients[i].fd = new_socket;
            clients[i].mode = 0;
            clients[i].last_activity = time(NULL);
            printf("[Server] Client %d connected from %s:%d\n",
                   i + 1, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            return;
        }
    }

    const char *msg = "ERROR: Server full\n";
    send(new_socket, msg, strlen(msg), 0);
    close(new_socket);
}

static void handle_client_data(int idx, client_t *clients, int *current_mode) {
    int client_fd = clients[idx].fd;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    ssize_t valread = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (valread < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        perror("[Server] recv error");
        close(client_fd);
        clients[idx].fd = -1;
        return;
    } 
    
    if (valread == 0) {
        printf("[Server] Client %d disconnected\n", idx + 1);
        close(client_fd);
        clients[idx].fd = -1;
        return;
    }

    buffer[strcspn(buffer, "\r\n")] = 0;
    if (strlen(buffer) == 0) return;

    clients[idx].last_activity = time(NULL);
    char response[BUFFER_SIZE];
    memset(response, 0, sizeof(response));

    if (strcmp(buffer, "GET_TEMP") == 0) {
        double temp = TEMP_MIN + (rand() % (int)(TEMP_VARIATION * 10)) / 10.0;
        snprintf(response, sizeof(response), "%.1f\n", temp);
        printf("[Server] Client %d: GET_TEMP -> %.1f\n", idx + 1, temp);
    } else if (strcmp(buffer, "GET_HUMIDITY") == 0) {
        double humidity = HUMI_MIN + (rand() % (int)(HUMI_VARIATION * 10)) / 10.0;
        snprintf(response, sizeof(response), "%.1f\n", humidity);
        printf("[Server] Client %d: GET_HUMIDITY -> %.1f\n", idx + 1, humidity);
    } else if (strncmp(buffer, "SET_MODE ", 9) == 0) {
        int mode = atoi(buffer + 9);
        clients[idx].mode = mode;
        *current_mode = mode;
        snprintf(response, sizeof(response), "OK\n");
        printf("[Server] Client %d: SET_MODE %d -> OK\n", idx + 1, mode);
    } else if (strcmp(buffer, "QUIT") == 0) {
        snprintf(response, sizeof(response), "BYE\n");
        send(client_fd, response, strlen(response), 0);
        printf("[Server] Client %d sent QUIT\n", idx + 1);
        close(client_fd);
        clients[idx].fd = -1;
        return;
    } else {
        snprintf(response, sizeof(response), "ERROR: Unknown command\n");
        printf("[Server] Client %d: Unknown command '%s'\n", idx + 1, buffer);
    }

    send(client_fd, response, strlen(response), 0);
}

static void do_broadcast(client_t *clients, int current_mode) {
    int connected_count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0) connected_count++;
    }

    if (connected_count == 0) return;

    double temp = TEMP_MIN + (rand() % (int)(TEMP_VARIATION * 10)) / 10.0;
    double humidity = HUMI_MIN + (rand() % (int)(HUMI_VARIATION * 10)) / 10.0;
    char broadcast_msg[BUFFER_SIZE];

    snprintf(broadcast_msg, sizeof(broadcast_msg),
             "[BROADCAST] Temp=%.1f Humidity=%.1f Mode=%d Clients=%d\n",
             temp, humidity, current_mode, connected_count);

    printf("[Server] Broadcasting to %d clients: Temp=%.1f Humidity=%.1f Mode=%d Clients=%d\n",
           connected_count, temp, humidity, current_mode, connected_count);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0) {
            ssize_t sent = send(clients[i].fd, broadcast_msg, strlen(broadcast_msg), MSG_DONTWAIT);
            if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                close(clients[i].fd);
                clients[i].fd = -1;
            }
        }
    }
}

int main(void) {
    srand(time(NULL));
    signal(SIGINT, sigint_handler);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("[Server] socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("[Server] setsockopt failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[Server] bind failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, MAX_CLIENTS) < 0) {
        perror("[Server] listen failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("[Server] Listening on localhost:%d\n", SERVER_PORT);

    client_t clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].mode = 0;
        clients[i].last_activity = 0;
    }

    time_t last_broadcast = time(NULL);
    int current_mode = 1;

    while (keep_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listen_fd, &readfds);
        int max_fd = listen_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd > 0) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd) {
                    max_fd = clients[i].fd;
                }
            }
        }

        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0 && errno != EINTR) {
            perror("[Server] select error");
            break;
        }

        if (activity > 0) {
            if (FD_ISSET(listen_fd, &readfds)) {
                handle_new_connection(listen_fd, clients);
            }

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd > 0 && FD_ISSET(clients[i].fd, &readfds)) {
                    handle_client_data(i, clients, &current_mode);
                }
            }
        }

        time_t now = time(NULL);
        if (now - last_broadcast >= BROADCAST_INTERVAL) {
            last_broadcast = now;
            do_broadcast(clients, current_mode);
        }
    }

    printf("\n[Server] Shutting down...\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0) close(clients[i].fd);
    }
    close(listen_fd);
    printf("[Server] Shutdown complete.\n");

    return 0;
}
