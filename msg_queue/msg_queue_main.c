/*
 * 文件名：msg_queue_practice.c
 * 作用：实验三——Linux 进程管理——消息队列通信（练习/验收版）
 *
 * 功能要求（按教材实验要求实现）：
 * 1. 创建三个线程：sender1、sender2、receiver
 * 2. sender1 和 sender2 通过消息队列向 receiver 发送消息
 * 3. sender1 输入 exit 后，发送 "end1"，等待收到 "over1" 后结束
 * 4. sender2 输入 exit 后，发送 "end2"，等待收到 "over2" 后结束
 * 5. receiver 收到 "end1" 后返回 "over1"
 * 6. receiver 收到 "end2" 后返回 "over2"
 * 7. 最后删除消息队列
 *
 * 说明：
 * 这里使用的是 Linux System V 消息队列：
 *   msgget()  创建/打开消息队列
 *   msgsnd()  发送消息
 *   msgrcv()  接收消息
 *   msgctl()  删除消息队列
 *
 * 线程说明：
 * - sender1 线程：负责输入并发送普通消息，最终发送 end1
 * - sender2 线程：负责输入并发送普通消息，最终发送 end2
 * - receiver线程：负责接收消息，并在需要时返回 over1 / over2
 *
 * 为什么还要加互斥锁？
 * 因为 sender1 和 sender2 都会从同一个终端 stdin 读输入。
 * 如果不加锁，两个线程可能同时打印提示、同时读输入，界面会非常乱。
 * 所以这里用 pthread_mutex_t 保护“终端输入和提示输出”这部分。
 */

#include <stdio.h>      // printf, perror, fgets
#include <stdlib.h>     // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <string.h>     // memset, strcmp, strncpy, strcspn
#include <pthread.h>    // pthread_create, pthread_join, pthread_mutex_xxx
#include <sys/ipc.h>    // IPC_PRIVATE, IPC_CREAT
#include <sys/msg.h>    // msgget, msgsnd, msgrcv, msgctl
#include <sys/types.h>  // 基本系统类型
#include <unistd.h>     // getpid

/* 普通文本最大长度 */
#define TEXT_SIZE 128

/*
 * 消息类型设计
 *
 * System V 消息队列要求：
 * 消息结构体的第一个成员必须是 long 类型的 mtype
 *
 * 我们约定：
 * 1   -> sender1 发给 receiver 的消息
 * 2   -> sender2 发给 receiver 的消息
 * 101 -> receiver 发给 sender1 的应答
 * 102 -> receiver 发给 sender2 的应答
 */
#define TYPE_SENDER1_TO_RECV  1
#define TYPE_SENDER2_TO_RECV  2
#define TYPE_RECV_TO_SENDER1  101
#define TYPE_RECV_TO_SENDER2  102

/*
 * System V 消息队列中的消息结构
 *
 * 注意：
 * 1. 第一个字段必须是 long mtype
 * 2. msgsnd / msgrcv 传递的“消息长度”，不包含 mtype
 */
typedef struct {
    long mtype;             // 消息类型
    char text[TEXT_SIZE];   // 消息正文
} Message;

/* 全局消息队列 ID，三个线程都要用 */
int g_msqid = -1;

/*
 * 终端输入互斥锁
 *
 * sender1 和 sender2 都会从标准输入读取字符串。
 * 如果两个线程同时读 stdin，会非常混乱，所以要加锁保护。
 */
pthread_mutex_t g_input_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * 功能：从终端安全读取一行字符串
 * 参数：
 *   prompt - 提示语
 *   buf    - 保存输入内容的缓冲区
 *   size   - 缓冲区大小
 *
 * 说明：
 * - 这里给“打印提示 + fgets读取输入”整体加锁
 * - 保证同一时刻只有一个 sender 线程与用户交互
 * - 输入后的换行符 '\n' 会被去掉
 */
void read_line_safely(const char *prompt, char *buf, size_t size) {
    pthread_mutex_lock(&g_input_mutex);

    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buf, size, stdin) == NULL) {
        /*
         * 如果 fgets 失败（比如用户按了 Ctrl+D），
         * 这里把它当成输入了 exit，方便程序正常收尾。
         */
        strncpy(buf, "exit", size - 1);
        buf[size - 1] = '\0';
    } else {
        /* 去掉 fgets 读进来的末尾换行符 */
        buf[strcspn(buf, "\n")] = '\0';
    }

    pthread_mutex_unlock(&g_input_mutex);
}

/*
 * 功能：发送一条消息到消息队列
 * 参数：
 *   type - 消息类型
 *   text - 消息正文
 */
void send_message(long type, const char *text) {
    Message msg;

    memset(&msg, 0, sizeof(msg));
    msg.mtype = type;
    strncpy(msg.text, text, sizeof(msg.text) - 1);
    msg.text[sizeof(msg.text) - 1] = '\0';

    /*
     * msgsnd 第三个参数是消息正文长度，不包括 mtype
     */
    if (msgsnd(g_msqid, &msg, sizeof(msg.text), 0) == -1) {
        perror("msgsnd failed");
        pthread_exit(NULL);
    }
}

/*
 * 功能：接收指定类型的一条消息
 * 参数：
 *   type - 希望接收的消息类型
 *   out  - 保存接收到的消息正文
 *
 * 说明：
 * - 如果 type=101，就只接收发给 sender1 的应答
 * - 如果 type=102，就只接收发给 sender2 的应答
 * - 如果 type=0，则表示接收队列中的第一条消息（不按类型筛选）
 */
void receive_message(long type, char *out) {
    Message msg;

    memset(&msg, 0, sizeof(msg));

    if (msgrcv(g_msqid, &msg, sizeof(msg.text), type, 0) == -1) {
        perror("msgrcv failed");
        pthread_exit(NULL);
    }

    strncpy(out, msg.text, TEXT_SIZE - 1);
    out[TEXT_SIZE - 1] = '\0';
}

/*
 * sender1 线程函数
 *
 * 行为：
 * 1. 循环从终端读取输入
 * 2. 如果输入不是 exit，就把内容通过消息队列发给 receiver
 * 3. 如果输入是 exit，就发送 "end1"
 * 4. 然后阻塞等待 receiver 返回 "over1"
 * 5. 收到 over1 后结束线程
 */
void *sender1_thread(void *arg) {
    char input[TEXT_SIZE];
    char reply[TEXT_SIZE];

    (void)arg;  // 避免未使用参数警告

    printf("[sender1] thread started.\n");

    while (1) {
        read_line_safely("[sender1] Please input a message (type exit to finish sender1): ",
                         input, sizeof(input));

        if (strcmp(input, "exit") == 0) {
            printf("[sender1] input is exit, now send \"end1\" to receiver.\n");
            send_message(TYPE_SENDER1_TO_RECV, "end1");

            printf("[sender1] waiting for reply \"over1\"...\n");
            receive_message(TYPE_RECV_TO_SENDER1, reply);

            printf("[sender1] received reply: %s\n", reply);
            printf("[sender1] thread finished.\n");
            break;
        } else {
            printf("[sender1] sending normal message: %s\n", input);
            send_message(TYPE_SENDER1_TO_RECV, input);
        }
    }

    return NULL;
}

/*
 * sender2 线程函数
 *
 * 行为与 sender1 类似：
 * - 普通输入：发送普通消息
 * - 输入 exit：发送 "end2"，等待 "over2"，然后结束
 */
void *sender2_thread(void *arg) {
    char input[TEXT_SIZE];
    char reply[TEXT_SIZE];

    (void)arg;

    printf("[sender2] thread started.\n");

    while (1) {
        read_line_safely("[sender2] Please input a message (type exit to finish sender2): ",
                         input, sizeof(input));

        if (strcmp(input, "exit") == 0) {
            printf("[sender2] input is exit, now send \"end2\" to receiver.\n");
            send_message(TYPE_SENDER2_TO_RECV, "end2");

            printf("[sender2] waiting for reply \"over2\"...\n");
            receive_message(TYPE_RECV_TO_SENDER2, reply);

            printf("[sender2] received reply: %s\n", reply);
            printf("[sender2] thread finished.\n");
            break;
        } else {
            printf("[sender2] sending normal message: %s\n", input);
            send_message(TYPE_SENDER2_TO_RECV, input);
        }
    }

    return NULL;
}

/*
 * receiver 线程函数
 *
 * 功能：
 * 1. 不断从消息队列接收 sender1 / sender2 发来的消息
 * 2. 普通消息：直接显示
 * 3. 收到 "end1"：回发 "over1"
 * 4. 收到 "end2"：回发 "over2"
 * 5. 当 end1 和 end2 都收到后，删除消息队列并结束
 */
void *receiver_thread(void *arg) {
    Message msg;
    int end1_received = 0;
    int end2_received = 0;

    (void)arg;

    printf("[receiver] thread started.\n");

    while (!(end1_received && end2_received)) {
        memset(&msg, 0, sizeof(msg));

        /*
         * type = 0 表示接收消息队列中的第一条消息
         * 不限定消息类型
         */
        if (msgrcv(g_msqid, &msg, sizeof(msg.text), 0, 0) == -1) {
            perror("receiver msgrcv failed");
            pthread_exit(NULL);
        }

        /*
         * 根据 mtype 判断消息是 sender1 发来的还是 sender2 发来的
         */
        if (msg.mtype == TYPE_SENDER1_TO_RECV) {
            if (strcmp(msg.text, "end1") == 0) {
                printf("[receiver] received termination message from sender1: end1\n");
                printf("[receiver] now send reply \"over1\" to sender1.\n");
                send_message(TYPE_RECV_TO_SENDER1, "over1");
                end1_received = 1;
            } else {
                printf("[receiver] normal message from sender1: %s\n", msg.text);
            }
        } else if (msg.mtype == TYPE_SENDER2_TO_RECV) {
            if (strcmp(msg.text, "end2") == 0) {
                printf("[receiver] received termination message from sender2: end2\n");
                printf("[receiver] now send reply \"over2\" to sender2.\n");
                send_message(TYPE_RECV_TO_SENDER2, "over2");
                end2_received = 1;
            } else {
                printf("[receiver] normal message from sender2: %s\n", msg.text);
            }
        } else {
            /*
             * 理论上本实验不会收到别的类型
             * 这里保留打印，便于调试
             */
            printf("[receiver] received an unknown message type: %ld, text=%s\n",
                   msg.mtype, msg.text);
        }
    }

    /*
     * 两个 sender 都结束后，receiver 删除消息队列
     */
    printf("[receiver] both sender1 and sender2 have finished.\n");
    printf("[receiver] now delete the message queue.\n");

    if (msgctl(g_msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl(IPC_RMID) failed");
        pthread_exit(NULL);
    }

    printf("[receiver] message queue deleted successfully.\n");
    printf("[receiver] thread finished.\n");

    return NULL;
}

int main() {
    pthread_t tid_sender1;
    pthread_t tid_sender2;
    pthread_t tid_receiver;

    /*
     * 创建 System V 消息队列
     *
     * 这里直接使用 IPC_PRIVATE：
     * - 会创建一个全新的、唯一的消息队列
     * - 由于本实验的 3 个线程都在同一个进程里，
     *   所以它们共享全局变量 g_msqid 即可
     */
    g_msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    if (g_msqid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

    printf("[main] message queue created successfully. msqid = %d\n", g_msqid);
    printf("[main] process pid = %d\n", getpid());

    /*
     * 创建三个线程
     */
    if (pthread_create(&tid_sender1, NULL, sender1_thread, NULL) != 0) {
        perror("pthread_create sender1 failed");
        msgctl(g_msqid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&tid_sender2, NULL, sender2_thread, NULL) != 0) {
        perror("pthread_create sender2 failed");
        msgctl(g_msqid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&tid_receiver, NULL, receiver_thread, NULL) != 0) {
        perror("pthread_create receiver failed");
        msgctl(g_msqid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    /*
     * 等待三个线程结束
     */
    pthread_join(tid_sender1, NULL);
    pthread_join(tid_sender2, NULL);
    pthread_join(tid_receiver, NULL);

    /*
     * 销毁互斥锁
     */
    pthread_mutex_destroy(&g_input_mutex);

    printf("[main] all threads finished. Program exit normally.\n");

    return 0;
}