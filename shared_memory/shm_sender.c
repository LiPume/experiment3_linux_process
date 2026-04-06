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
    int shmid = shmget(SHM_KEY, sizeof(struct shm_data), 0666 | IPC_CREAT);
    if (shmid < 0) {
        perror("shmget failed");
        exit(1);
    }

    struct shm_data *shm = (struct shm_data *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    shm->flag = 0;

    while (1) {
        while (shm->flag == 1);  // 等待 receiver 读完

        printf("Input: ");
        fgets(shm->text, 1024, stdin);

        shm->flag = 1;  // 写完

        if (strncmp(shm->text, "exit", 4) == 0)
            break;
    }

    shmdt(shm);
    return 0;
}