#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_KEY 5678
#define DATA_SIZE 4096

struct shm_data {
    volatile int flag;
    char text[DATA_SIZE];
};

int main() {
    int shmid = shmget(SHM_KEY, sizeof(struct shm_data), 0666 | IPC_CREAT);
    if (shmid < 0) {
        perror("receiver shmget failed");
        exit(1);
    }

    struct shm_data *shm = (struct shm_data *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("receiver shmat failed");
        exit(1);
    }

    while (1) {
        // 延迟测试：收到1字节消息
        if (shm->flag == 1) {
            volatile char c = shm->text[0];
            (void)c;
            shm->flag = 2;
        }
        // 吞吐量测试：收到大块数据
        else if (shm->flag == 3) {
            volatile char c = shm->text[0];
            (void)c;
            shm->flag = 4;
        }
        // 结束
        else if (shm->flag == 9) {
            break;
        }
    }

    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}
