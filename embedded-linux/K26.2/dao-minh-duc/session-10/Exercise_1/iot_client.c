#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "iot_server.h"

int main(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return 1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        close(sock);
        return 1;
    }

    printf("Connected to IoT Server on localhost:%d. Type commands (GET_TEMP, GET_HUMIDITY, SET_MODE <n>, QUIT):\n", SERVER_PORT);

    char send_buf[BUFFER_SIZE];
    char recv_buf[BUFFER_SIZE];

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        int max_fd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select error");
            break;
        }

        // Nhận dữ liệu/broadcast từ server
        if (FD_ISSET(sock, &readfds)) {
            memset(recv_buf, 0, sizeof(recv_buf));
            int valread = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
            if (valread <= 0) {
                printf("Server disconnected.\n");
                break;
            }
            printf("%s", recv_buf);
        }

        // Lấy lệnh nhập từ bàn phím
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(send_buf, sizeof(send_buf), stdin) == NULL) {
                break;
            }

            send(sock, send_buf, strlen(send_buf), 0);

            send_buf[strcspn(send_buf, "\r\n")] = 0;
            if (strcmp(send_buf, "quit") == 0 || strcmp(send_buf, "QUIT") == 0) {
                break;
            }
        }
    }

    close(sock);
    return 0;
}
