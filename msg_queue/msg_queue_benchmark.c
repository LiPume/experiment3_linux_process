#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define LATENCY_ROUNDS 1000
#define THROUGHPUT_ROUNDS 5000
#define DATA_SIZE 4096

struct msgbuf {
    long mtype;
    char mtext[DATA_SIZE];
};

double get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000.0 + ts.tv_nsec / 1000.0;
}

int main() {
    key_t key = IPC_PRIVATE;
    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid < 0) {
        perror("msgget failed");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        msgctl(msgid, IPC_RMID, NULL);
        exit(1);
    }

    if (pid == 0) {
        struct msgbuf msg;
        struct msgbuf ack;

        while (1) {
            ssize_t n = msgrcv(msgid, &msg, sizeof(msg.mtext), 1, 0);
            if (n < 0) {
                perror("child msgrcv failed");
                exit(1);
            }

            if (n == 1) {
                ack.mtype = 2;
                ack.mtext[0] = 'A';
                if (msgsnd(msgid, &ack, 1, 0) < 0) {
                    perror("child msgsnd ack failed");
                    exit(1);
                }
            }

            if (strncmp(msg.mtext, "END", 3) == 0) {
                break;
            }
        }

        exit(0);
    }

    struct msgbuf msg;
    struct msgbuf ack;
    memset(&msg, 0, sizeof(msg));
    memset(&ack, 0, sizeof(ack));

    double total_latency = 0.0;

    // ===== 延迟测试：1字节消息来回一次 =====
    msg.mtype = 1;
    msg.mtext[0] = 'X';

    for (int i = 0; i < LATENCY_ROUNDS; i++) {
        double start = get_time_us();

        if (msgsnd(msgid, &msg, 1, 0) < 0) {
            perror("parent msgsnd latency failed");
            msgctl(msgid, IPC_RMID, NULL);
            exit(1);
        }

        if (msgrcv(msgid, &ack, sizeof(ack.mtext), 2, 0) < 0) {
            perror("parent msgrcv ack failed");
            msgctl(msgid, IPC_RMID, NULL);
            exit(1);
        }

        double end = get_time_us();
        total_latency += (end - start);
    }

    double avg_latency = total_latency / LATENCY_ROUNDS / 2.0;

    // ===== 吞吐量测试：大消息单向发送 =====
    msg.mtype = 1;
    memset(msg.mtext, 'a', DATA_SIZE);

    double start = get_time_us();

    for (int i = 0; i < THROUGHPUT_ROUNDS; i++) {
        if (msgsnd(msgid, &msg, DATA_SIZE, 0) < 0) {
            perror("parent msgsnd throughput failed");
            msgctl(msgid, IPC_RMID, NULL);
            exit(1);
        }
    }

    double end = get_time_us();

    double seconds = (end - start) / 1000000.0;
    double throughput = (DATA_SIZE * THROUGHPUT_ROUNDS) / seconds / (1024.0 * 1024.0);

    // 发送结束标志
    strcpy(msg.mtext, "END");
    if (msgsnd(msgid, &msg, 4, 0) < 0) {
        perror("parent msgsnd END failed");
    }

    wait(NULL);
    msgctl(msgid, IPC_RMID, NULL);

    printf("\n===== Message Queue Benchmark =====\n");
    printf("Average latency : %.2f us\n", avg_latency);
    printf("Throughput      : %.2f MB/s\n", throughput);

    return 0;
}
