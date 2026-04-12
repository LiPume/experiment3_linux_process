#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#define LATENCY_ROUNDS 1000
#define THROUGHPUT_ROUNDS 5000
#define DATA_SIZE 4096

double get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000.0 + ts.tv_nsec / 1000.0;
}

int main() {
    int pipe1[2]; // 父 -> 子
    int pipe2[2]; // 子 -> 父（仅用于延迟测试确认）

    if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
        perror("pipe error");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");
        exit(1);
    }

    if (pid == 0) {
        char buf[DATA_SIZE];

        close(pipe1[1]);
        close(pipe2[0]);

        while (1) {
            int n = read(pipe1[0], buf, DATA_SIZE);
            if (n <= 0) {
                break;
            }

            // 小包：延迟测试，回1字节确认
            if (n == 1) {
                char ack = 'A';
                write(pipe2[1], &ack, 1);
            }
            // 大包：吞吐量测试，只读不回写，避免pipe2写满死锁
        }

        close(pipe1[0]);
        close(pipe2[1]);
        exit(0);
    }

    char buf[DATA_SIZE];
    memset(buf, 'a', DATA_SIZE);

    close(pipe1[0]);
    close(pipe2[1]);

    double total_latency = 0.0;

    // ===== 延迟测试 =====
    for (int i = 0; i < LATENCY_ROUNDS; i++) {
        char ack;
        double start = get_time_us();

        write(pipe1[1], buf, 1);
        read(pipe2[0], &ack, 1);

        double end = get_time_us();
        total_latency += (end - start);
    }

    double avg_latency = total_latency / LATENCY_ROUNDS / 2.0;

    // ===== 吞吐量测试 =====
    double start = get_time_us();

    for (int i = 0; i < THROUGHPUT_ROUNDS; i++) {
        if (write(pipe1[1], buf, DATA_SIZE) != DATA_SIZE) {
            perror("write failed");
            break;
        }
    }

    double end = get_time_us();

    double seconds = (end - start) / 1000000.0;
    double throughput = (DATA_SIZE * THROUGHPUT_ROUNDS) / seconds / (1024.0 * 1024.0);

    close(pipe1[1]);
    close(pipe2[0]);

    wait(NULL);

    printf("\n===== Pipe Benchmark =====\n");
    printf("Average latency : %.2f us\n", avg_latency);
    printf("Throughput      : %.2f MB/s\n", throughput);

    return 0;
}
