#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "event_monitor.h"

int main(void) {
    char cmd[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    printf("Unix Socket Client connected to %s.\nCommands: STATUS, START, STOP, EXIT\n", SOCKET_PATH);

    while (1) {
        printf("> ");
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;

        cmd[strcspn(cmd, "\r\n")] = 0;
        if (strlen(cmd) == 0) continue;

        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket error");
            break;
        }

        struct sockaddr_un un_addr;
        memset(&un_addr, 0, sizeof(un_addr));
        un_addr.sun_family = AF_UNIX;
        strncpy(un_addr.sun_path, SOCKET_PATH, sizeof(un_addr.sun_path) - 1);

        if (connect(sock, (struct sockaddr *)&un_addr, sizeof(un_addr)) < 0) {
            perror("connect failed");
            close(sock);
            break;
        }

        strcat(cmd, "\n");
        send(sock, cmd, strlen(cmd), 0);

        memset(response, 0, sizeof(response));
        ssize_t valread = recv(sock, response, sizeof(response) - 1, 0);
        if (valread > 0) {
            printf("%s", response);
        }

        close(sock);

        cmd[strcspn(cmd, "\r\n")] = 0;
        if (strcasecmp(cmd, "EXIT") == 0) {
            break;
        }
    }

    return 0;
}
