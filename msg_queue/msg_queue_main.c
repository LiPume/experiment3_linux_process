/*
 * 文件名：msg_queue_main.c
 * 模块：消息队列通信
 * 负责人：
 * 功能：创建 sender1、sender2、receiver 三个线程进行通信
 * 关键函数/系统调用：pthread_create(), msgget(), msgsnd(), msgrcv(), msgctl()
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// TODO: 定义消息结构体
// TODO: 定义消息队列相关变量

void *sender1(void *arg) {
    // TODO
    return NULL;
}

void *sender2(void *arg) {
    // TODO
    return NULL;
}

void *receiver(void *arg) {
    // TODO
    return NULL;
}

int main() {
    pthread_t t1, t2, t3;

    // TODO: 创建消息队列
    // TODO: 创建三个线程
    // TODO: 等待线程结束
    // TODO: 删除消息队列

    return 0;
}
