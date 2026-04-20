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
sem_t full;           // 队列中已有消息数
sem_t empty;          // 队列剩余空间
sem_t s_display;      // 控制发送线程输入显示
sem_t r_display;      // 控制接收线程执行
sem_t mq_ready;       // 通知主线程：消息队列已创建

// ---------- 消息队列 ----------
int msgid = -1;       // 全局消息队列 id，由 sender1 创建
key_t key = 100;      // 消息队列 key

pthread_mutex_t print_mutex;

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

        // 接收 sender1 / sender2 发来的消息，统一用类型1
        ret = msgrcv(msgid, &buf, sizeof(buf.msg), 1, 0);
        if (ret == -1)
        {
            perror("receiver msgrcv error");
            pthread_exit(NULL);
        }

        pthread_mutex_lock(&print_mutex);
        printf("receiver: %s\n", buf.msg);
        printf("-----------------------------------------\n");
        pthread_mutex_unlock(&print_mutex);

        // 收到 end1，回复 over1 给 sender1（消息类型2）
        if (strcmp(buf.msg, "end1") == 0 && !end1_flag)
        {
            struct msgbuff reply;
            memset(&reply, 0, sizeof(reply));
            reply.msg_type = 2;
            strcpy(reply.msg, "over1");

            ret = msgsnd(msgid, &reply, sizeof(reply.msg), 0);
            if (ret == -1)
            {
                perror("receiver msgsnd over1 error");
                pthread_exit(NULL);
            }

            end1_flag = 1;
        }
        // 收到 end2，回复 over2 给 sender2（消息类型3）
        else if (strcmp(buf.msg, "end2") == 0 && !end2_flag)
        {
            struct msgbuff reply;
            memset(&reply, 0, sizeof(reply));
            reply.msg_type = 3;
            strcpy(reply.msg, "over2");

            ret = msgsnd(msgid, &reply, sizeof(reply.msg), 0);
            if (ret == -1)
            {
                perror("receiver msgsnd over2 error");
                pthread_exit(NULL);
            }

            end2_flag = 1;
        }

        sem_post(&empty);
        sem_post(&s_display);
    }

    pthread_mutex_lock(&print_mutex);
    printf("receiver: sender1 and sender2 are all over.\n");
    pthread_mutex_unlock(&print_mutex);

    // 由 receiver 删除消息队列
    if (msgctl(msgid, IPC_RMID, NULL) == -1)
    {
        perror("receiver msgctl IPC_RMID error");
    }
    else
    {
        pthread_mutex_lock(&print_mutex);
        printf("receiver: message queue deleted.\n");
        pthread_mutex_unlock(&print_mutex);
    }

    pthread_exit(NULL);
}

// sender1线程：创建消息队列
void *sender1(void *arg)
{
    struct msgbuff buf;
    int flag = 1;
    int ret;

    // sender1 创建消息队列
    msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1)
    {
        perror("sender1 msgget error");
        pthread_exit(NULL);
    }

    pthread_mutex_lock(&print_mutex);
    printf("sender1: message queue created successfully, msgid = %d\n", msgid);
    pthread_mutex_unlock(&print_mutex);

    // 通知主线程：消息队列已创建
    sem_post(&mq_ready);

    while (flag)
    {
        memset(&buf, 0, sizeof(buf));

        sem_wait(&s_display);
        sem_wait(&empty);

        pthread_mutex_lock(&print_mutex);
        printf("sender1> ");
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);
        if (scanf("%99s", buf.msg) != 1)
        {
            pthread_mutex_lock(&print_mutex);
            printf("sender1 input error\n");
            pthread_mutex_unlock(&print_mutex);
            sem_post(&empty);
            sem_post(&s_display);
            continue;
        }

        buf.msg_type = 1;

        if (strcmp(buf.msg, "exit") == 0)
        {
            strcpy(buf.msg, "end1");
            flag = 0;
        }
        else
        {
            strcat(buf.msg, " --s1");
        }

        ret = msgsnd(msgid, &buf, sizeof(buf.msg), 0);
        if (ret == -1)
        {
            perror("sender1 msgsnd error");
            sem_post(&empty);
            sem_post(&s_display);
            pthread_exit(NULL);
        }

        sem_post(&full);
        sem_post(&r_display);
    }

    // 等待 receiver 回复 over1
    memset(&buf, 0, sizeof(buf));
    ret = msgrcv(msgid, &buf, sizeof(buf.msg), 2, 0);
    if (ret == -1)
    {
        perror("sender1 msgrcv over1 error");
        pthread_exit(NULL);
    }

    pthread_mutex_lock(&print_mutex);
    printf("sender1 received reply: %s\n", buf.msg);
    pthread_mutex_unlock(&print_mutex);
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

        pthread_mutex_lock(&print_mutex);
        printf("sender2> ");
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);
        if (scanf("%99s", buf.msg) != 1)
        {
            pthread_mutex_lock(&print_mutex);
            printf("sender2 input error\n");
            pthread_mutex_unlock(&print_mutex);
            sem_post(&empty);
            sem_post(&s_display);
            continue;
        }

        buf.msg_type = 1;

        if (strcmp(buf.msg, "exit") == 0)
        {
            strcpy(buf.msg, "end2");
            flag = 0;
        }
        else
        {
            strcat(buf.msg, " --s2");
        }

        ret = msgsnd(msgid, &buf, sizeof(buf.msg), 0);
        if (ret == -1)
        {
            perror("sender2 msgsnd error");
            sem_post(&empty);
            sem_post(&s_display);
            pthread_exit(NULL);
        }

        sem_post(&full);
        sem_post(&r_display);
    }

    // 等待 receiver 回复 over2
    memset(&buf, 0, sizeof(buf));
    ret = msgrcv(msgid, &buf, sizeof(buf.msg), 3, 0);
    if (ret == -1)
    {
        perror("sender2 msgrcv over2 error");
        pthread_exit(NULL);
    }

    pthread_mutex_lock(&print_mutex);
    printf("sender2 received reply: %s\n", buf.msg);
    pthread_mutex_unlock(&print_mutex);
    pthread_exit(NULL);
}

int main()
{
    int ret;

    // 初始化信号量
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, 5);
    sem_init(&s_display, 0, 1);
    sem_init(&r_display, 0, 0);
    sem_init(&mq_ready, 0, 0);

    pthread_mutex_init(&print_mutex, NULL);

    // 先创建 sender1，让它负责创建消息队列
    ret = pthread_create(&s1, NULL, sender1, NULL);
    if (ret != 0)
    {
        printf("create sender1 error!\n");
        exit(1);
    }

    // 等待 sender1 创建好消息队列
    sem_wait(&mq_ready);

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

    pthread_join(s1, NULL);
    pthread_join(s2, NULL);
    pthread_join(r, NULL);

    sem_destroy(&full);
    sem_destroy(&empty);
    sem_destroy(&s_display);
    sem_destroy(&r_display);
    sem_destroy(&mq_ready);

    pthread_mutex_destroy(&print_mutex);

    return 0;
}
