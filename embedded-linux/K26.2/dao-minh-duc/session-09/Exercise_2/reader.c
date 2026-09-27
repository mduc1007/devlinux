#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>

#include "device_cfg.h"

static device_cfg_t *cfg = MAP_FAILED;

void sigint_handler(int sig) {
    (void)sig;
    printf("\n[Config Reader] Exiting cleanup...\n");
    if (cfg != MAP_FAILED) {
        munmap(cfg, sizeof(device_cfg_t));
    }
    exit(0);
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
    signal(SIGINT, sigint_handler);

    // 1. Mở file ở chế độ chỉ đọc O_RDONLY
    int fd = open(CONFIG_FILE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("[Reader] Cannot open /tmp/device.cfg. Make sure config-writer is running or file exists");
        exit(1);
    }

    // 2. Ánh xạ mmap chỉ đọc (PROT_READ)
    cfg = (device_cfg_t *)mmap(NULL, sizeof(device_cfg_t),
                              PROT_READ, MAP_SHARED, fd, 0);
    if (cfg == MAP_FAILED) {
        perror("[Reader] mmap failed");
        close(fd);
        exit(1);
    }

    close(fd);

    printf("[Config Reader] Polling %s every 2s...\n", CONFIG_FILE_PATH);

    // 3. Vòng lặp đọc dữ liệu liên tục từ vùng nhớ mmap
    while (1) {
        printf("baud_rate=%-6d  sampling_rate=%-3d Hz  log_level=%s\n",
               cfg->baud_rate,
               cfg->sampling_rate_hz,
               get_log_level_str(cfg->log_level));
        sleep(2);
    }

    return 0;
}
