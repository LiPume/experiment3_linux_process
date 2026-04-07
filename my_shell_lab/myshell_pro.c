#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_HISTORY 10

int main() {
    char buf[100];
    char history[MAX_HISTORY][100]; // 创新点2：存储历史记录的数组
    int history_count = 0;

    while (1) {
        printf("HDU-Pro-Shell > ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL) break;
        buf[strcspn(buf, "\n")] = 0;

        if (strlen(buf) == 0) continue;

        // --- 创新点2：保存历史记录 ---
        if (history_count < MAX_HISTORY) {
            strcpy(history[history_count++], buf);
        } else {
            // 满了就挪动位置（简易覆盖）
            for(int i=0; i<MAX_HISTORY-1; i++) strcpy(history[i], history[i+1]);
            strcpy(history[MAX_HISTORY-1], buf);
        }

        // --- 创新点1：实现内建命令 cd ---
        if (strncmp(buf, "cd ", 3) == 0) {
            char *path = buf + 3; // 获取 "cd " 之后的路径
            if (chdir(path) != 0) {
                perror("cd 失败");
            }
            continue; // 直接跳过后面的 fork
        }

        // --- 创新点2：实现内建命令 history ---
        if (strcmp(buf, "history") == 0) {
            for (int i = 0; i < history_count; i++) {
                printf("%d: %s\n", i + 1, history[i]);
            }
            continue;
        }

        if (strcmp(buf, "exit") == 0) break;

        // --- 核心逻辑：执行外部命令 ---
        pid_t pid = fork();
        if (pid == 0) {
            // --- 创新点3：升级为 execvp ---
            // 构造参数数组，execvp 需要 char* 数组
            char *args[2];
            args[0] = buf;   // 命令名
            args[1] = NULL;  // 必须以 NULL 结尾

            // 尝试执行。如果 buf 是 "ls"，它会自动去系统路径找
            // 如果 buf 是 "./cmd1"，它也会在当前目录找
            if (execvp(args[0], args) == -1) {
                printf("错误：未找到命令 '%s'\n", buf);
                exit(1);
            }
        } else {
            wait(NULL);
        }
    }
    return 0;
}
