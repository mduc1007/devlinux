#ifndef IOT_SERVER_H
#define IOT_SERVER_H

#include <time.h>

#define SERVER_PORT        9999
#define MAX_CLIENTS        10
#define BROADCAST_INTERVAL 5  /* seconds */
#define BUFFER_SIZE        1024

typedef struct {
    int    fd;           /* socket file descriptor, -1 nếu trống */
    int    mode;         /* 0=idle, 1=active, 2=alert */
    time_t last_activity;
} client_t;

#endif // IOT_SERVER_H
