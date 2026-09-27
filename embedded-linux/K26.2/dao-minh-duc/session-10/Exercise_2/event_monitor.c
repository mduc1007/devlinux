#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

#include "event_monitor.h"

static volatile sig_atomic_t keep_running = 1;

static void sig_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

int main(void) {
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    int total_events = 0;
    int is_active = 1;
    time_t start_time = time(NULL);

    /* 1. Initialize FIFO */
    if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) {
        perror("[Monitor] mkfifo failed");
        exit(EXIT_FAILURE);
    }
    
    int fifo_fd = open(FIFO_PATH, O_RDWR | O_NONBLOCK);
    if (fifo_fd < 0) {
        perror("[Monitor] open FIFO failed");
        exit(EXIT_FAILURE);
    }

    /* 2. Initialize Unix Domain Socket Listener */
    unlink(SOCKET_PATH);
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("[Monitor] socket failed");
        close(fifo_fd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un un_addr;
    memset(&un_addr, 0, sizeof(un_addr));
    un_addr.sun_family = AF_UNIX;
    strncpy(un_addr.sun_path, SOCKET_PATH, sizeof(un_addr.sun_path) - 1);

    if (bind(socket_fd, (struct sockaddr *)&un_addr, sizeof(un_addr)) < 0) {
        perror("[Monitor] bind socket failed");
        close(fifo_fd);
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fd, 5) < 0) {
        perror("[Monitor] listen socket failed");
        close(fifo_fd);
        close(socket_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }

    /* 3. Initialize File Size Tracking */
    int status_fd = open(FILE_PATH, O_RDWR | O_CREAT, 0666);
    off_t last_file_size = 0;

    if (status_fd < 0) {
        perror("[Monitor] open FILE_PATH failed");
    } else {
        struct stat st;
        if (fstat(status_fd, &st) == 0) {
            last_file_size = st.st_size;
        } else {
            perror("[Monitor] fstat initial failed");
        }
    }

    /* 4. Setup pollfd array */
    struct pollfd fds[NUM_POLL_FDS];
    fds[FD_FIFO].fd = fifo_fd;
    fds[FD_FIFO].events = POLLIN;

    fds[FD_SOCKET_LISTENER].fd = socket_fd;
    fds[FD_SOCKET_LISTENER].events = POLLIN;

    printf("[Monitor] Listening on %s\n", SOCKET_PATH);
    printf("[Monitor] Monitoring %s (FIFO) and %s\n", FIFO_PATH, FILE_PATH);

    while (keep_running) {
        int ret = poll(fds, NUM_POLL_FDS, POLL_TIMEOUT_MS);

        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("[Monitor] poll error");
            break;
        }

        if (ret == 0) {
            printf("[HEARTBEAT] Monitor alive, events_seen=%d\n", total_events);
        } else {
            /* Handle FIFO Events */
            if (fds[FD_FIFO].revents & POLLIN) {
                char buf[BUFFER_SIZE];
                memset(buf, 0, sizeof(buf));
                ssize_t bytes_read = read(fifo_fd, buf, sizeof(buf) - 1);
                if (bytes_read > 0) {
                    buf[strcspn(buf, "\r\n")] = 0;
                    if (is_active && strlen(buf) > 0) {
                        printf("[FIFO_EVENT] %s\n", buf);
                        total_events++;
                    }
                }
            }

            /* Handle Unix Control Socket Events */
            if (fds[FD_SOCKET_LISTENER].revents & POLLIN) {
                int client_fd = accept(socket_fd, NULL, NULL);
                if (client_fd >= 0) {
                    char buf[BUFFER_SIZE];
                    memset(buf, 0, sizeof(buf));
                    
                    ssize_t valread;
                    do {
                        valread = recv(client_fd, buf, sizeof(buf) - 1, 0);
                    } while (valread < 0 && errno == EINTR);

                    if (valread > 0) {
                        buf[strcspn(buf, "\r\n")] = 0;
                        char resp[BUFFER_SIZE];
                        memset(resp, 0, sizeof(resp));

                        if (strcmp(buf, "STATUS") == 0) {
                            int uptime = (int)(time(NULL) - start_time);
                            snprintf(resp, sizeof(resp), "Total events: %d, Uptime: %d seconds\n",
                                     total_events, uptime);
                        } else if (strcmp(buf, "START") == 0) {
                            is_active = 1;
                            snprintf(resp, sizeof(resp), "OK\n");
                        } else if (strcmp(buf, "STOP") == 0) {
                            is_active = 0;
                            snprintf(resp, sizeof(resp), "OK\n");
                        } else if (strcmp(buf, "EXIT") == 0) {
                            snprintf(resp, sizeof(resp), "BYE\n");
                        } else {
                            snprintf(resp, sizeof(resp), "ERROR: Unknown command\n");
                        }
                        send(client_fd, resp, strlen(resp), 0);
                    }
                    close(client_fd);
                }
            }
        }

        /* Check File Size Changes */
        if (status_fd >= 0 && is_active) {
            struct stat st;
            if (fstat(status_fd, &st) < 0) {
                perror("[Monitor] fstat error");
            } else if (st.st_size != last_file_size) {
                printf("[FILE_EVENT] %s size changed to %ld bytes\n", FILE_PATH, (long)st.st_size);
                last_file_size = st.st_size;
                total_events++;
            }
        }
    }

    printf("\n[Monitor] Shutdown complete.\n");
    close(fifo_fd);
    close(socket_fd);
    if (status_fd >= 0) close(status_fd);
    unlink(FIFO_PATH);
    unlink(SOCKET_PATH);

    return 0;
}
