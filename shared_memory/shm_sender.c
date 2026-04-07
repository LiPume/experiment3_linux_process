/*
 * 文件名：shm_sender_practice.c
 * 作用：实验三——Linux 进程管理——共享内存通信（sender）
 *
 * 功能要求（按教材实验要求实现）：
 * 1. sender 创建共享内存
 * 2. sender 从终端输入一条字符串
 * 3. sender 将字符串写入共享内存
 * 4. sender 等待 receiver 的应答 "over"
 * 5. sender 显示应答
 * 6. sender 最后删除共享内存
 *
 * 说明：
 * 这里使用 System V 共享内存：
 *   shmget()  创建/获取共享内存
 *   shmat()   连接共享内存到当前进程地址空间
 *   shmdt()   断开连接
 *   shmctl()  删除共享内存
 *
 * 同步方式：
 * 使用共享内存中的 status 状态字段进行简单同步。
 * 这是课程实验里最容易理解和讲解的一种实现方式。
 */

#include <stdio.h>      // printf, perror, fgets
#include <stdlib.h>     // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <string.h>     // memset, strcspn, strncpy, strcmp
#include <sys/ipc.h>    // ftok
#include <sys/shm.h>    // shmget, shmat, shmdt, shmctl
#include <sys/types.h>  // 基本系统类型
#include <unistd.h>     // usleep

/* 共享内存 key 文件路径 */
#define SHM_KEY_PATH "shmfile"

/* 共享内存项目号，用于 ftok */
#define SHM_PROJ_ID 65

/* 消息文本最大长度 */
#define TEXT_SIZE 128

/* 共享内存中的状态值定义 */
#define STATUS_EMPTY      0   // 空闲，没有有效消息
#define STATUS_WRITTEN    1   // sender 已写入消息，等待 receiver 读取
#define STATUS_REPLIED    2   // receiver 已写回 "over"，等待 sender 读取
#define STATUS_FINISHED   3   // 通信结束

/*
 * 共享内存中的数据结构
 *
 * status: 用来做同步控制
 * text  : 用来存放真正的消息内容
 */
typedef struct {
    int status;
    char text[TEXT_SIZE];
} SharedData;

int main() {
    key_t key;
    int shmid;
    SharedData *shm_ptr;
    char input[TEXT_SIZE];

    /*
     * 第一步：通过 ftok 生成一个 key
     *
     * sender 和 receiver 必须使用相同的 key，
     * 才能定位到同一块共享内存。
     *
     * 注意：
     * 这里要求文件 shared_memory/shmfile 必须存在。
     * 你可以提前 touch 一下这个文件。
     */
    key = ftok(SHM_KEY_PATH, SHM_PROJ_ID);
    if (key == -1) {
        perror("ftok failed");
        printf("Please make sure file \"%s\" exists.\n", SHM_KEY_PATH);
        exit(EXIT_FAILURE);
    }

    /*
     * 第二步：创建共享内存
     *
     * 参数说明：
     * 1. key：共享内存键值
     * 2. sizeof(SharedData)：共享内存大小
     * 3. IPC_CREAT | 0666：不存在就创建，权限为 0666
     */
    shmid = shmget(key, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    /*
     * 第三步：把共享内存连接到当前进程地址空间
     *
     * 成功后返回共享内存首地址
     */
    shm_ptr = (SharedData *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    /*
     * 初始化共享内存内容
     */
    shm_ptr->status = STATUS_EMPTY;
    memset(shm_ptr->text, 0, sizeof(shm_ptr->text));

    printf("[sender] shared memory created and attached successfully.\n");
    printf("[sender] shmid = %d\n", shmid);
    printf("[sender] Please input a message to send to receiver: ");
    fflush(stdout);

    /*
     * 从终端读取一行字符串
     */
    if (fgets(input, sizeof(input), stdin) == NULL) {
        perror("fgets failed");
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    /* 去掉结尾换行符 */
    input[strcspn(input, "\n")] = '\0';

    /*
     * 第四步：把消息写入共享内存
     */
    strncpy(shm_ptr->text, input, sizeof(shm_ptr->text) - 1);
    shm_ptr->text[sizeof(shm_ptr->text) - 1] = '\0';

    /*
     * 写完消息后，把状态改成 STATUS_WRITTEN
     * 表示：receiver 可以来读了
     */
    shm_ptr->status = STATUS_WRITTEN;

    printf("[sender] message has been written into shared memory: \"%s\"\n", shm_ptr->text);
    printf("[sender] now waiting for receiver's reply...\n");

    /*
     * 第五步：等待 receiver 回写 "over"
     *
     * 这里采用轮询等待：
     * 只要状态还不是 STATUS_REPLIED，就继续等待
     *
     * usleep(100000) 表示睡眠 100ms，
     * 避免死循环空转占满 CPU。
     */
    while (shm_ptr->status != STATUS_REPLIED) {
        usleep(100000);
    }

    /*
     * 第六步：读取 receiver 的应答
     */
    printf("[sender] received reply from receiver: \"%s\"\n", shm_ptr->text);

    /*
     * 第七步：通知通信结束
     */
    shm_ptr->status = STATUS_FINISHED;

    /*
     * 第八步：断开共享内存连接
     */
    if (shmdt(shm_ptr) == -1) {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }

    /*
     * 第九步：删除共享内存
     *
     * IPC_RMID 表示将该共享内存段标记为删除。
     * 当没有进程再连接它时，系统会真正回收。
     */
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl(IPC_RMID) failed");
        exit(EXIT_FAILURE);
    }

    printf("[sender] shared memory detached and deleted successfully.\n");
    printf("[sender] program finished.\n");

    return 0;
}