#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#include "sensor_shm.h"

static int shmid = -1;
static sensor_data_t *shm_ptr = NULL;

// Trình xử lý tín hiệu SIGINT (Ctrl+C) để dọn dẹp tài nguyên IPC
void sigint_handler(int sig) {
    (void)sig;
    printf("\n[Daemon] Cleaning up shared memory. Goodbye.\n");
    if (shm_ptr != NULL && shm_ptr != (void *)-1) {
        shmdt(shm_ptr);
    }
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    exit(0);
}

// Đọc loadavg từ /proc/loadavg để tính nhiệt độ giả lập
double get_cpu_temp(void) {
    FILE *fp = fopen("/proc/loadavg", "r");
    if (!fp) {
        perror("[Daemon] Failed to open /proc/loadavg");
        return 40.0;
    }
    double load1 = 0.0;
    if (fscanf(fp, "%lf", &load1) != 1) {
        load1 = 0.0;
    }
    fclose(fp);
    return 40.0 + (load1 * 10.0);
}

// Đọc MemTotal và MemFree từ /proc/meminfo để tính % RAM đã dùng
double get_ram_usage(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("[Daemon] Failed to open /proc/meminfo");
        return 0.0;
    }

    char line[128];
    long mem_total = 0, mem_free = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %ld kB", &mem_total) == 1) continue;
        if (sscanf(line, "MemFree: %ld kB", &mem_free) == 1) continue;
    }
    fclose(fp);

    if (mem_total == 0) return 0.0;
    return ((double)(mem_total - mem_free) / mem_total) * 100.0;
}

int main(void) {
    // 1. Đăng ký SIGINT handler
    signal(SIGINT, sigint_handler);

    // 2. Tạo Shared Memory region
    shmid = shmget(SHM_KEY, sizeof(sensor_data_t), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("[Daemon] shmget failed");
        exit(1);
    }
    printf("[Daemon] Shared memory created. Key=0x%X\n", SHM_KEY);

    // 3. Attach vào Shared Memory
    shm_ptr = (sensor_data_t *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("[Daemon] shmat failed");
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);
    }

    // 4. Vòng lặp cập nhật dữ liệu mỗi 2 giây
    while (1) {
        double temp = get_cpu_temp();
        double ram = get_ram_usage();
        time_t now = time(NULL);

        shm_ptr->timestamp = now;
        shm_ptr->cpu_temp = temp;
        shm_ptr->ram_used_pct = ram;

        printf("[Daemon] Written: temp=%.2f ram=%.2f%%\n", temp, ram);
        sleep(2);
    }

    return 0;
}
