#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>

#define SHM_KEY 5678
#define LATENCY_ROUNDS 1000
#define THROUGHPUT_ROUNDS 5000
#define DATA_SIZE 4096

struct shm_data {
    volatile int flag;
    char text[DATA_SIZE];
};

double get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000.0 + ts.tv_nsec / 1000.0;
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(struct shm_data), 0666 | IPC_CREAT);
    if (shmid < 0) {
        perror("sender shmget failed");
        exit(1);
    }

    struct shm_data *shm = (struct shm_data *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("sender shmat failed");
        exit(1);
    }

    shm->flag = 0;

    double total_latency = 0.0;

    // ===== 延迟测试：1字节来回确认 =====
    for (int i = 0; i < LATENCY_ROUNDS; i++) {
        while (shm->flag != 0);

        double start = get_time_us();

        shm->text[0] = 'X';
        shm->flag = 1;   // 数据已写好，等待receiver处理

        while (shm->flag != 2);

        double end = get_time_us();
        total_latency += (end - start);

        shm->flag = 0;   // 准备下一轮
    }

    double avg_latency = total_latency / LATENCY_ROUNDS / 2.0;

    // ===== 吞吐量测试：大块数据单向发送 =====
    memset(shm->text, 'a', DATA_SIZE);

    double start = get_time_us();

    for (int i = 0; i < THROUGHPUT_ROUNDS; i++) {
        while (shm->flag != 0);

        shm->flag = 3;   // 表示大块数据已写入
        while (shm->flag != 4);
        shm->flag = 0;
    }

    double end = get_time_us();

    double seconds = (end - start) / 1000000.0;
    double throughput = (DATA_SIZE * THROUGHPUT_ROUNDS) / seconds / (1024.0 * 1024.0);

    // 结束标志
    while (shm->flag != 0);
    strcpy(shm->text, "END");
    shm->flag = 9;

    printf("\n===== Shared Memory Benchmark =====\n");
    printf("Average latency : %.2f us\n", avg_latency);
    printf("Throughput      : %.2f MB/s\n", throughput);

    shmdt(shm);
    return 0;
}
