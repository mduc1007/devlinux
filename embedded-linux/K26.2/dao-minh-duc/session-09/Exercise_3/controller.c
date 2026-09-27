#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>

#define SHM_NAME "/device_shm"

typedef struct {
    pthread_mutex_t mutex; /* Mutex nằm trực tiếp trong bộ nhớ dùng chung */
    int status;            /* 0 = OFF, 1 = ON */
} device_state_t;

int main(void) {
    // 1. Tạo hoặc mở POSIX shared memory object
    int fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("[Controller] shm_open failed");
        exit(1);
    }

    // 2. Thiết lập kích thước cho shared memory region
    if (ftruncate(fd, sizeof(device_state_t)) < 0) {
        perror("[Controller] ftruncate failed");
        close(fd);
        shm_unlink(SHM_NAME);
        exit(1);
    }

    // 3. Ánh xạ mmap với quyền đọc/ghi
    device_state_t *state = (device_state_t *)mmap(NULL, sizeof(device_state_t),
                                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (state == MAP_FAILED) {
        perror("[Controller] mmap failed");
        close(fd);
        shm_unlink(SHM_NAME);
        exit(1);
    }

    // Có thể đóng file descriptor sau khi mmap thành công
    close(fd);

    // 4. Khởi tạo Mutex với thuộc tính PTHREAD_PROCESS_SHARED
    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0) {
        perror("[Controller] pthread_mutexattr_init failed");
        goto cleanup;
    }

    if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
        perror("[Controller] pthread_mutexattr_setpshared failed");
        pthread_mutexattr_destroy(&attr);
        goto cleanup;
    }

    if (pthread_mutex_init(&state->mutex, &attr) != 0) {
        perror("[Controller] pthread_mutex_init failed");
        pthread_mutexattr_destroy(&attr);
        goto cleanup;
    }
    pthread_mutexattr_destroy(&attr);

    // Trạng thái ban đầu
    state->status = 0;

    printf("[Controller] Shared memory ready. Commands: on / off / quit\n");

    char cmd[32];
    while (1) {
        printf("> ");
        if (scanf("%31s", cmd) != 1) break;

        if (strcmp(cmd, "quit") == 0) {
            break;
        } else if (strcmp(cmd, "on") == 0) {
            pthread_mutex_lock(&state->mutex);
            state->status = 1;
            pthread_mutex_unlock(&state->mutex);
            printf("[Controller] Command sent: ON\n");
        } else if (strcmp(cmd, "off") == 0) {
            pthread_mutex_lock(&state->mutex);
            state->status = 0;
            pthread_mutex_unlock(&state->mutex);
            printf("[Controller] Command sent: OFF\n");
        } else {
            printf("[Controller] Unknown command! Use 'on', 'off', or 'quit'.\n");
        }
    }

cleanup:
    printf("[Controller] Cleaning up. Goodbye.\n");
    pthread_mutex_destroy(&state->mutex);
    munmap(state, sizeof(device_state_t));
    shm_unlink(SHM_NAME);

    return 0;
}
