#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_HISTORY 10

int main() {
    char buf[100];
    char history[MAX_HISTORY][100];
    int history_count = 0;

    while (1) {
        printf("HDU-Mixed-Shell > ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL) break;
        buf[strcspn(buf, "\n")] = 0; // 必须在这里切断换行符
        if (strlen(buf) == 0) continue;

        // 历史记录逻辑
        if (history_count < MAX_HISTORY) {
            strcpy(history[history_count++], buf);
        } else {
            for(int i=0; i<MAX_HISTORY-1; i++) strcpy(history[i], history[i+1]);
            strcpy(history[MAX_HISTORY-1], buf);
        }

        if (strcmp(buf, "exit") == 0) break;
        if (strcmp(buf, "history") == 0) {
            for (int i = 0; i < history_count; i++) printf("%d: %s\n", i + 1, history[i]);
            continue;
        }
        if (strncmp(buf, "cd ", 3) == 0) {
            if (chdir(buf + 3) != 0) perror("cd 失败");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            char path[128];
            memset(path, 0, sizeof(path));
            strcpy(path, "./");
            strcat(path, buf);

            // [调试信息] 老师，看这里，这是为了确认路径拼接是否正确
            // printf("[Debug] 正在尝试访问路径: %s\n", path); 

            if (access(path, X_OK) == 0) {
                execl(path, buf, NULL);
            } else {
                char *args[] = {buf, NULL};
                execvp(args[0], args);
            }

            printf("错误：未找到命令 '%s'\n", buf);
            exit(1);
        } else {
            wait(NULL);
        }
    }
    return 0;
}
