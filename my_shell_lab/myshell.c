#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

double get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000.0 + ts.tv_nsec / 1000.0;
}

int main() {
    char buf[100];

    while (1) {
        printf("HDU-Computer-Science-Shell > ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL) break;

        buf[strcspn(buf, "\n")] = 0;

        if (strcmp(buf, "exit") == 0) {
            printf("正在退出 Shell... 再见！\n");
            break;
        }

        if (strlen(buf) == 0) continue;

        double start_time, end_time;
        start_time = get_time_us();

        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork 出错了");
        }
        else if (pid == 0) {
            char path[120] = "./";
            strcat(path, buf);

            if (execl(path, buf, NULL) == -1) {
                printf("错误：找不到命令 '%s'，请确保输入的是 cmd1, cmd2 或 cmd3。\n", buf);
                exit(1);
            }
        }
        else {
            wait(NULL);
            end_time = get_time_us();
            printf("[性能] 命令 '%s' 延迟: %.2f us\n", buf, end_time - start_time);
        }
    }

    return 0;
}
