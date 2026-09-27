#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>

#define SHM_NAME "/device_shm"

typedef struct {
    pthread_mutex_t mutex;
    int status; /* 0 = OFF, 1 = ON */
} device_state_t;

static device_state_t *state = MAP_FAILED;

void sigint_handler(int sig) {
    (void)sig;
    printf("\n[Device] Detaching from %s...\n", SHM_NAME);
    if (state != MAP_FAILED) {
        munmap(state, sizeof(device_state_t));
    }
    exit(0);
}

int main(void) {
    signal(SIGINT, sigint_handler);

    // 1. Kết nối tới shared memory đã được tạo bởi controller (không dùng O_CREAT)
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd < 0) {
        perror("[Device] shm_open failed. Make sure controller is running first");
        exit(1);
    }

    // 2. Ánh xạ mmap
    state = (device_state_t *)mmap(NULL, sizeof(device_state_t),
                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (state == MAP_FAILED) {
        perror("[Device] mmap failed");
        close(fd);
        exit(1);
    }

    close(fd);

    printf("[Device] Attached to %s\n", SHM_NAME);

    // 3. Vòng lặp kiểm tra trạng thái mỗi 1 giây
    while (1) {
        pthread_mutex_lock(&state->mutex);
        int current_status = state->status;
        pthread_mutex_unlock(&state->mutex);

        if (current_status == 1) {
            printf("[Device] Status: ON  — Running...\n");
        } else {
            printf("[Device] Status: OFF — Idle.\n");
        }

        sleep(1);
    }

    return 0;
}
