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
        
        if (strcmp(buf, "exit") == 0) break;

        pid_t pid = fork();
        if (pid == 0) {
            // 自然的错误逻辑：顺手就把 cd 写在子进程里了
            if (strncmp(buf, "cd ", 3) == 0) {
                if (chdir(buf + 3) != 0) {
                    perror("cd failed");
                }
                exit(0); // 以为切换成功了，正常退出
            }
            
            char *args[] = {buf, NULL};
            execvp(args[0], args);
            printf("Command not found\n");
            exit(1);
        } else {
            wait(NULL);
        }
    }
    return 0;
}
