#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 128

int main() {
    char input[MAX_INPUT];
    pid_t pid;

    while (1) {
        printf("my_shell> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        // 去掉换行符
        input[strcspn(input, "\n")] = '\0';

        // 空输入，继续等待
        if (strlen(input) == 0) {
            continue;
        }

        // 输入 exit，退出 shell
        if (strcmp(input, "exit") == 0) {
            printf("Shell exited.\n");
            break;
        }

        // 只识别 cmd1 / cmd2 / cmd3
        if (strcmp(input, "cmd1") != 0 &&
            strcmp(input, "cmd2") != 0 &&
            strcmp(input, "cmd3") != 0) {
            printf("Command not found\n");
            continue;
        }

        pid = fork();

        if (pid < 0) {
            perror("fork failed");
            continue;
        } else if (pid == 0) {
            // 子进程执行相应命令
            if (strcmp(input, "cmd1") == 0) {
                execl("./cmd1", "cmd1", NULL);
            } else if (strcmp(input, "cmd2") == 0) {
                execl("./cmd2", "cmd2", NULL);
            } else if (strcmp(input, "cmd3") == 0) {
                execl("./cmd3", "cmd3", NULL);
            }

            // execl 失败才会执行到这里
            perror("execl failed");
            exit(1);
        } else {
            // 父进程等待子进程结束
            wait(NULL);
        }
    }

    return 0;
}