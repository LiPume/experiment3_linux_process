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
        buf[strcspn(buf, "\n")] = 0; 
        if (strlen(buf) == 0) continue;

        // 【致命错误】：早期代码没有任何边界检查，直接往里塞！
        // 超过10次就会操作非法内存
        strcpy(history[history_count], buf);
        history_count++;

        if (strcmp(buf, "exit") == 0) break;
        if (strcmp(buf, "history") == 0) {
            for (int i = 0; i < history_count; i++) printf("%d: %s\n", i + 1, history[i]);
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            char *args[] = {buf, NULL};
            execvp(args[0], args);
            printf("错误：未找到命令\n");
            exit(1);
        } else {
            wait(NULL);
        }
    }
    return 0;
}
