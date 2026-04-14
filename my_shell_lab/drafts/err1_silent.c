#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char buf[100];
    while (1) {
        printf("HDU-Mixed-Shell > ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL) break;
        buf[strcspn(buf, "\n")] = 0; 
        if (strlen(buf) == 0) continue;
        if (strcmp(buf, "exit") == 0) break;

        pid_t pid = fork();
        if (pid == 0) {
            char *args[] = {buf, NULL};
            execvp(args[0], args);
            // 【致命错误模拟】：execvp 失败了，但这里没有任何 perror 提示！
            // 就直接悄悄死掉了
            exit(1);
        } else {
            wait(NULL);
        }
    }
    return 0;
}
