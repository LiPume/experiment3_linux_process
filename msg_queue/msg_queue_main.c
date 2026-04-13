#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>

// ---------- 线程 ----------
pthread_t s1, s2, r;

// ---------- 信号量 ----------
sem_t mutex;          // 互斥访问消息队列
sem_t full;           // 队列中已有消息数
sem_t empty;          // 队列剩余空间
sem_t over;           // 保留，不删除
sem_t s_display;      // 控制发送线程输入显示
sem_t r_display;      // 控制接收线程执行

// ---------- 消息队列 ----------
int msgid = -1;       // 全局消息队列 id，由 sender1 创建
key_t key = 100;      // 消息队列 key，保留在主程序中

// ---------- 消息结构 ----------
struct msgbuff
{
    long msg_type;
    char msg[100];
};

// receiver线程
void *receive(void *arg)
{
    struct msgbuff buf;
    int end1_flag = 0;
    int end2_flag = 0;
    int ret;

    while (!(end1_flag && end2_flag))
    {
        memset(&buf, 0, sizeof(buf));

        sem_wait(&r_display);
        sem_wait(&full);
        sem_wait(&mutex);

        // 接收 sender1 / sender2 发来的普通消息，统一用类型1
        ret = msgrcv(msgid, &buf, sizeof(buf.msg), 1, 0);
        if (ret == -1)
        {
            perror("receiver msgrcv error");
            sem_post(&mutex);
            pthread_exit(NULL);
        }

        printf("receiver: %s\n", buf.msg);
        printf("-----------------------------------------\n");

        // 收到 end1，回复 over1 给 sender1（消息类型 2）
        if (strcmp(buf.msg, "end1") == 0 && !end1_flag)
        {
            struct msgbuff reply;
            memset(&reply, 0, sizeof(reply));
            reply.msg_type = 2;   // 专门给 sender1
            strcpy(reply.msg, "over1");

            ret = msgsnd(msgid, &reply, sizeof(reply.msg), 0);
            if (ret == -1)
            {
                perror("receiver msgsnd over1 error");
                sem_post(&mutex);
                pthread_exit(NULL);
            }

            end1_flag = 1;
        }
        // 收到 end2，回复 over2 给 sender2（消息类型 3）
        else if (strcmp(buf.msg, "end2") == 0 && !end2_flag)
        {
            struct msgbuff reply;
            memset(&reply, 0, sizeof(reply));
            reply.msg_type = 3;   // 专门给 sender2
            strcpy(reply.msg, "over2");

            ret = msgsnd(msgid, &reply, sizeof(reply.msg), 0);
            if (ret == -1)
            {
                perror("receiver msgsnd over2 error");
                sem_post(&mutex);
                pthread_exit(NULL);
            }

            end2_flag = 1;
        }

        sem_post(&mutex);
        sem_post(&empty);
        sem_post(&s_display);
    }

    printf("receiver: sender1 and sender2 are all over.\n");

    // 按题目要求：由 receiver 删除消息队列
    if (msgctl(msgid, IPC_RMID, NULL) == -1)
    {
        perror("receiver msgctl IPC_RMID error");
    }
    else
    {
        printf("receiver: message queue deleted.\n");
    }

    pthread_exit(NULL);
}

// sender1线程：创建消息队列
void *sender1(void *arg)
{
    struct msgbuff buf;
    int flag = 1;
    int ret;

    // 按题目要求：sender1 创建消息队列
    msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1)
    {
        perror("sender1 msgget error");
        pthread_exit(NULL);
    }

    printf("sender1: message queue created successfully, msgid = %d\n", msgid);

    while (flag)
    {
        memset(&buf, 0, sizeof(buf));

        sem_wait(&s_display);
        sem_wait(&empty);
        sem_wait(&mutex);

        printf("sender1> ");
        scanf("%99s", buf.msg);

        buf.msg_type = 1;

        if (strcmp(buf.msg, "exit") == 0)
        {
            strcpy(buf.msg, "end1");
            flag = 0;
        }
        else
        {
            strcat(buf.msg, " ——s1");
        }

        ret = msgsnd(msgid, &buf, sizeof(buf.msg), 0);
        if (ret == -1)
        {
            perror("sender1 msgsnd error");
            sem_post(&mutex);
            pthread_exit(NULL);
        }

        sem_post(&r_display);
        sem_post(&full);
        sem_post(&mutex);
    }

    // 等待 receiver 回复 over1
    memset(&buf, 0, sizeof(buf));
    ret = msgrcv(msgid, &buf, sizeof(buf.msg), 2, 0);   // 只接收类型2
    if (ret == -1)
    {
        perror("sender1 msgrcv over1 error");
        pthread_exit(NULL);
    }

    printf("sender1 received reply: %s\n", buf.msg);
    pthread_exit(NULL);
}

// sender2线程：共享 sender1 创建的消息队列
void *sender2(void *arg)
{
    struct msgbuff buf;
    int flag = 1;
    int ret;

    while (flag)
    {
        memset(&buf, 0, sizeof(buf));

        sem_wait(&s_display);
        sem_wait(&empty);
        sem_wait(&mutex);

        printf("sender2> ");
        scanf("%99s", buf.msg);

        buf.msg_type = 1;

        if (strcmp(buf.msg, "exit") == 0)
        {
            strcpy(buf.msg, "end2");
            flag = 0;
        }
        else
        {
            strcat(buf.msg, " ——s2");
        }

        ret = msgsnd(msgid, &buf, sizeof(buf.msg), 0);
        if (ret == -1)
        {
            perror("sender2 msgsnd error");
            sem_post(&mutex);
            pthread_exit(NULL);
        }

        sem_post(&r_display);
        sem_post(&full);
        sem_post(&mutex);
    }

    // 等待 receiver 回复 over2
    memset(&buf, 0, sizeof(buf));
    ret = msgrcv(msgid, &buf, sizeof(buf.msg), 3, 0);   // 只接收类型3
    if (ret == -1)
    {
        perror("sender2 msgrcv over2 error");
        pthread_exit(NULL);
    }

    printf("sender2 received reply: %s\n", buf.msg);
    pthread_exit(NULL);
}

int main()
{
    int ret;

    // 初始化信号量
    sem_init(&mutex, 0, 1);
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, 5);
    sem_init(&over, 0, 0);       // 保留，不删
    sem_init(&s_display, 0, 1);
    sem_init(&r_display, 0, 0);

    // 先创建 sender1，让它先创建消息队列
    ret = pthread_create(&s1, NULL, sender1, NULL);
    if (ret != 0)
    {
        printf("create sender1 error!\n");
        exit(1);
    }

    // 等待 sender1 创建好消息队列
    while (msgid == -1)
    {
        usleep(1000);
    }

    // 再创建 receiver 和 sender2
    ret = pthread_create(&r, NULL, receive, NULL);
    if (ret != 0)
    {
        printf("create receiver error!\n");
        exit(1);
    }

    ret = pthread_create(&s2, NULL, sender2, NULL);
    if (ret != 0)
    {
        printf("create sender2 error!\n");
        exit(1);
    }

    // 等待三个线程结束
    pthread_join(s1, NULL);
    pthread_join(s2, NULL);
    pthread_join(r, NULL);

    // 销毁信号量
    sem_destroy(&mutex);
    sem_destroy(&full);
    sem_destroy(&empty);
    sem_destroy(&over);
    sem_destroy(&s_display);
    sem_destroy(&r_display);

    return 0;
}