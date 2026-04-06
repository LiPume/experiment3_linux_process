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
sem_t mutex;
sem_t full, empty;
sem_t s_display, r_display;

// ---------- 消息结构 ----------
struct msgbuff {
    long msg_type;
    char msg[100];
};

// ================= receiver =================
void *receive(void *arg)
{
    struct msgbuff buf;
    int flag1 = 1, flag2 = 1;
    int msgid = *(int *)arg;

    while (flag1 || flag2)
    {
        memset(&buf, 0, sizeof(buf));

        sem_wait(&r_display);
        sem_wait(&full);
        sem_wait(&mutex);

        // 接收 sender 发来的消息（类型1）
        msgrcv(msgid, &buf, sizeof(buf.msg), 1, 0);

        printf("receive: %s\n", buf.msg);
        printf("-----------------------------\n");

        // sender1结束
        if (flag1 && strcmp(buf.msg, "end1") == 0)
        {
            strcpy(buf.msg, "over1");
            buf.msg_type = 2;
            msgsnd(msgid, &buf, sizeof(buf.msg), 0);
            flag1 = 0;
        }
        // sender2结束
        else if (flag2 && strcmp(buf.msg, "end2") == 0)
        {
            strcpy(buf.msg, "over2");
            buf.msg_type = 2;
            msgsnd(msgid, &buf, sizeof(buf.msg), 0);
            flag2 = 0;
        }

        sem_post(&mutex);
        sem_post(&empty);
        sem_post(&s_display);
    }

    printf("All senders finished!\n");
    return NULL;
}

// ================= sender1 =================
void *sender1(void *arg)
{
    struct msgbuff buf;
    int msgid = *(int *)arg;

    while (1)
    {
        memset(&buf, 0, sizeof(buf));

        sem_wait(&s_display);
        sem_wait(&empty);
        sem_wait(&mutex);

        printf("sender1> ");
        scanf("%s", buf.msg);

        buf.msg_type = 1;

        if (strcmp(buf.msg, "exit") == 0)
        {
            strcpy(buf.msg, "end1");
            msgsnd(msgid, &buf, sizeof(buf.msg), 0);

            sem_post(&r_display);
            sem_post(&full);
            sem_post(&mutex);
            break;
        }

        strcat(buf.msg, " --s1");
        msgsnd(msgid, &buf, sizeof(buf.msg), 0);

        sem_post(&r_display);
        sem_post(&full);
        sem_post(&mutex);
    }

    // 接收 over1
    msgrcv(msgid, &buf, sizeof(buf.msg), 2, 0);
    printf("sender1 receive: %s\n", buf.msg);

    return NULL;
}

// ================= sender2 =================
void *sender2(void *arg)
{
    struct msgbuff buf;
    int msgid = *(int *)arg;

    while (1)
    {
        memset(&buf, 0, sizeof(buf));

        sem_wait(&s_display);
        sem_wait(&empty);
        sem_wait(&mutex);

        printf("sender2> ");
        scanf("%s", buf.msg);

        buf.msg_type = 1;

        if (strcmp(buf.msg, "exit") == 0)
        {
            strcpy(buf.msg, "end2");
            msgsnd(msgid, &buf, sizeof(buf.msg), 0);

            sem_post(&r_display);
            sem_post(&full);
            sem_post(&mutex);
            break;
        }

        strcat(buf.msg, " --s2");
        msgsnd(msgid, &buf, sizeof(buf.msg), 0);

        sem_post(&r_display);
        sem_post(&full);
        sem_post(&mutex);
    }

    // 接收 over2
    msgrcv(msgid, &buf, sizeof(buf.msg), 2, 0);
    printf("sender2 receive: %s\n", buf.msg);

    return NULL;
}

// ================= main =================
int main()
{
    sem_init(&mutex, 0, 1);
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, 5);
    sem_init(&s_display, 0, 1);
    sem_init(&r_display, 0, 0);

    key_t key = 100;
    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1)
    {
        perror("msgget error");
        exit(1);
    }

    // receiver先启动
    pthread_create(&r, NULL, receive, &msgid);

    // sender1
    pthread_create(&s1, NULL, sender1, &msgid);
    pthread_join(s1, NULL);

    // sender2
    pthread_create(&s2, NULL, sender2, &msgid);
    pthread_join(s2, NULL);

    pthread_join(r, NULL);

    msgctl(msgid, IPC_RMID, NULL);

    return 0;
}
