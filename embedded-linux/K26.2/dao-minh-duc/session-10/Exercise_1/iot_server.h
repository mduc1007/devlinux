#ifndef IOT_SERVER_H
#define IOT_SERVER_H

#include <time.h>

#define SERVER_PORT        9999
#define MAX_CLIENTS        10
#define BROADCAST_INTERVAL 5  /* seconds */
#define BUFFER_SIZE        1024

/* Symbolic Constants for Sensors */
#define TEMP_MIN           25.0
#define TEMP_VARIATION     10.0
#define HUMI_MIN           30.0
#define HUMI_VARIATION     30.0

typedef struct {
    int    fd;           /* socket file descriptor, -1 nếu trống */
    int    mode;         /* 0=idle, 1=active, 2=alert */
    time_t last_activity;
} client_t;

#endif // IOT_SERVER_H
