/*
 * 文件名：shell_main.c
 * 模块：模拟Shell
 * 负责人：
 * 功能：读取用户输入，创建子进程并执行对应命令程序
 * 关键系统调用：fork(), execl(), wait()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char command[100];

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        // TODO: 读取用户输入
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }

        command[strcspn(command, "\n")] = '\0';

        // TODO: 判断是否为 exit
        if (strcmp(command, "exit") == 0) {
            break;
        }

        // TODO: fork 创建子进程
        // TODO: 子进程 execl 执行命令
        // TODO: 父进程 wait 等待子进程结束
        // TODO: 无效命令提示 Command not found
    }

    return 0;
}
