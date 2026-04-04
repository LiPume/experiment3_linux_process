/*
 * 文件名：shm_receiver_practice.c
 * 作用：实验三——Linux 进程管理——共享内存通信（receiver）
 *
 * 功能要求（按教材实验要求实现）：
 * 1. receiver 连接 sender 创建的共享内存
 * 2. receiver 读取 sender 写入的消息
 * 3. receiver 将消息显示到终端
 * 4. receiver 再通过共享内存写回 "over"
 * 5. receiver 结束
 *
 * 同步方式：
 * 使用共享内存中的 status 状态字段。
 */

#include <stdio.h>      // printf, perror
#include <stdlib.h>     // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <string.h>     // strcpy
#include <sys/ipc.h>    // ftok
#include <sys/shm.h>    // shmget, shmat, shmdt
#include <sys/types.h>  // 基本系统类型
#include <unistd.h>     // usleep

/* 共享内存 key 文件路径 */
#define SHM_KEY_PATH "shared_memory/shmfile"

/* 共享内存项目号，必须与 sender 完全一致 */
#define SHM_PROJ_ID 65

/* 消息文本最大长度 */
#define TEXT_SIZE 128

/* 状态值，必须与 sender 完全一致 */
#define STATUS_EMPTY      0
#define STATUS_WRITTEN    1
#define STATUS_REPLIED    2
#define STATUS_FINISHED   3

/*
 * 共享内存中的结构体
 * 必须与 sender 中定义完全一致
 */
typedef struct {
    int status;
    char text[TEXT_SIZE];
} SharedData;

int main() {
    key_t key;
    int shmid;
    SharedData *shm_ptr;

    /*
     * 第一步：通过 ftok 获取同一个 key
     */
    key = ftok(SHM_KEY_PATH, SHM_PROJ_ID);
    if (key == -1) {
        perror("ftok failed");
        printf("Please make sure file \"%s\" exists.\n", SHM_KEY_PATH);
        exit(EXIT_FAILURE);
    }

    /*
     * 第二步：获取已经存在的共享内存
     *
     * 这里不加 IPC_CREAT，
     * 因为 receiver 的角色是连接 sender 已经创建好的共享内存。
     */
    shmid = shmget(key, sizeof(SharedData), 0666);
    if (shmid == -1) {
        perror("shmget failed");
        printf("[receiver] maybe sender has not created shared memory yet.\n");
        exit(EXIT_FAILURE);
    }

    /*
     * 第三步：连接共享内存
     */
    shm_ptr = (SharedData *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    printf("[receiver] shared memory attached successfully.\n");
    printf("[receiver] waiting for sender's message...\n");

    /*
     * 第四步：等待 sender 写入消息
     */
    while (shm_ptr->status != STATUS_WRITTEN) {
        usleep(100000);
    }

    /*
     * 第五步：读取并显示 sender 的消息
     */
    printf("[receiver] received message from sender: \"%s\"\n", shm_ptr->text);

    /*
     * 第六步：写回应答 "over"
     */
    strcpy(shm_ptr->text, "over");

    /*
     * 把状态改成 STATUS_REPLIED
     * 表示：sender 可以来读应答了
     */
    shm_ptr->status = STATUS_REPLIED;

    printf("[receiver] reply \"over\" has been written into shared memory.\n");
    printf("[receiver] waiting for sender to finish...\n");

    /*
     * 第七步：等待 sender 把状态改成 STATUS_FINISHED
     */
    while (shm_ptr->status != STATUS_FINISHED) {
        usleep(100000);
    }

    /*
     * 第八步：断开共享内存连接
     *
     * receiver 不负责删除共享内存，
     * 删除动作由 sender 完成。
     */
    if (shmdt(shm_ptr) == -1) {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }

    printf("[receiver] shared memory detached successfully.\n");
    printf("[receiver] program finished.\n");

    return 0;
}