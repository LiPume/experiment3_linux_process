/*
 * 文件名：pipe_main.c
 * 模块：管道通信
 * 负责人：
 * 功能：父进程创建管道并与3个子进程通信
 * 关键系统调用：pipe(), fork(), read(), write(), close(), wait()
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    int fd[2];

    // TODO: 创建管道
    // TODO: 创建3个子进程
    // TODO: 子进程通过管道写入消息
    // TODO: 父进程等待子进程完成
    // TODO: 父进程从管道读取消息
    // TODO: 分析阻塞型读写现象

    return 0;
}
