#include <stdio.h>
#include <sys/shm.h>
#include <string.h>
#include <stdlib.h>

#define SHM_KEY 1234

struct shm_data {
    int flag;        // 0: 可写, 1: 可读
    char text[1024];
};

int main() {
    int shmid = shmget(SHM_KEY, sizeof(struct shm_data), 0666);
    if (shmid < 0) {
        perror("shmget failed");
        exit(1);
    }

    struct shm_data *shm = (struct shm_data *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    while (1) {
        // 等待 sender 写入
        while (shm->flag == 0);

        // 输出接收到的数据
        printf("Received: %s", shm->text);

        // 标记为已读
        shm->flag = 0;

        // 如果是 exit，则退出
        if (strncmp(shm->text, "exit", 4) == 0)
            break;
    }

    // 解除映射
    shmdt(shm);

    // 删除共享内存（只需要 receiver 做）
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
