#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "device_cfg.h"

void print_config(const device_cfg_t *cfg) {
    printf("\nCurrent: baud_rate=%d sampling_rate=%d log_level=%d\n",
           cfg->baud_rate, cfg->sampling_rate_hz, cfg->log_level);
}

const char* get_log_level_str(int level) {
    switch (level) {
        case 0: return "OFF";
        case 1: return "ERROR";
        case 2: return "INFO";
        case 3: return "DEBUG";
        default: return "UNKNOWN";
    }
}

int main(void) {
    // 1. Mở hoặc tạo file /tmp/device.cfg
    int fd = open(CONFIG_FILE_PATH, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("[Writer] open failed");
        exit(1);
    }

    // 2. Thiết lập kích thước file vừa đủ cho struct
    if (ftruncate(fd, sizeof(device_cfg_t)) < 0) {
        perror("[Writer] ftruncate failed");
        close(fd);
        exit(1);
    }

    // 3. Ánh xạ file vào bộ nhớ
    device_cfg_t *cfg = (device_cfg_t *)mmap(NULL, sizeof(device_cfg_t),
                                             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (cfg == MAP_FAILED) {
        perror("[Writer] mmap failed");
        close(fd);
        exit(1);
    }

    // Sau khi mmap thành công có thể đóng file descriptor
    close(fd);

    printf("[Config Writer] Loaded %s\n", CONFIG_FILE_PATH);

    // Giá trị mặc định nếu file mới tạo
    if (cfg->baud_rate == 0) {
        cfg->baud_rate = 9600;
        cfg->sampling_rate_hz = 100;
        cfg->log_level = 2; // INFO
        msync(cfg, sizeof(device_cfg_t), MS_SYNC);
    }

    char choice[32];
    while (1) {
        print_config(cfg);
        printf("\nSelect field to update [baud/rate/log/quit]: ");
        if (scanf("%31s", choice) != 1) break;

        if (strcmp(choice, "quit") == 0) {
            break;
        } else if (strcmp(choice, "baud") == 0) {
            int val;
            printf("Select baud rate [9600/115200/460800]: ");
            if (scanf("%d", &val) == 1 && (val == 9600 || val == 115200 || val == 460800)) {
                cfg->baud_rate = val;
                msync(cfg, sizeof(device_cfg_t), MS_SYNC);
                printf("[Updated] baud_rate = %d\n", val);
            } else {
                printf("[Error] Invalid baud rate!\n");
            }
        } else if (strcmp(choice, "rate") == 0) {
            int val;
            printf("Enter sampling rate Hz (1-1000): ");
            if (scanf("%d", &val) == 1 && val >= 1 && val <= 1000) {
                cfg->sampling_rate_hz = val;
                msync(cfg, sizeof(device_cfg_t), MS_SYNC);
                printf("[Updated] sampling_rate_hz = %d\n", val);
            } else {
                printf("[Error] Invalid sampling rate!\n");
            }
        } else if (strcmp(choice, "log") == 0) {
            int val;
            printf("Select log level [0=OFF, 1=ERROR, 2=INFO, 3=DEBUG]: ");
            if (scanf("%d", &val) == 1 && val >= 0 && val <= 3) {
                cfg->log_level = val;
                msync(cfg, sizeof(device_cfg_t), MS_SYNC);
                printf("[Updated] log_level = %s (%d)\n", get_log_level_str(val), val);
            } else {
                printf("[Error] Invalid log level!\n");
            }
        } else {
            printf("[Error] Unknown field! Use baud, rate, log, or quit.\n");
        }
    }

    // 4. Giải phóng ánh xạ mmap
    if (munmap(cfg, sizeof(device_cfg_t)) < 0) {
        perror("[Writer] munmap failed");
    }

    printf("[Writer] Exiting...\n");
    return 0;
}
