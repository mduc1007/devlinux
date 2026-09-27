#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sensor_shm.h"

int main(void) {
    // 1. Kết nối đến vùng Shared Memory đã tạo (Không dùng IPC_CREAT)
    int shmid = shmget(SHM_KEY, sizeof(sensor_data_t), 0666);
    if (shmid < 0) {
        fprintf(stderr, "Daemon is not running.\n");
        exit(1);
    }

    // 2. Attach để truy cập dữ liệu
    sensor_data_t *shm_ptr = (sensor_data_t *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // 3. Đọc và in báo cáo ra stdout
    printf("[Sensor Report]\n");
    printf("Timestamp : %ld\n", (long)shm_ptr->timestamp);
    printf("CPU Temp  : %.2f C\n", shm_ptr->cpu_temp);
    printf("RAM Used  : %.2f %%\n", shm_ptr->ram_used_pct);

    // 4. Detach bộ nhớ trước khi thoát
    if (shmdt(shm_ptr) < 0) {
        perror("shmdt failed");
        exit(1);
    }

    return 0;
}
