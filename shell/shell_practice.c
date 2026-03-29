#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    while (1) {
        printf("my_shell> ");
        fflush(stdout);

        // 读取一行输入
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // Ctrl+D 或输入流结束
            printf("\n");
            break;
        }

        // 去掉末尾换行符
        input[strcspn(input, "\n")] = '\0';

        // 空命令直接继续
        if (strlen(input) == 0) {
            continue;
        }

        // 内建命令：exit
        if (strcmp(input, "exit") == 0) {
            break;
        }

        // 按空格拆分参数
        int i = 0;
        char *token = strtok(input, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        // 创建子进程
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            continue;
        } else if (pid == 0) {
            // 子进程执行命令
            execvp(args[0], args);

            // execvp 失败才会执行到这里
            perror("exec failed");
            exit(1);
        } else {
            // 父进程等待子进程结束
            wait(NULL);
        }
    }

    return 0;
}