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
        
        // 【致命错误模拟】：故意漏掉了下面这行去换行符的代码
        // buf[strcspn(buf, "\n")] = 0; 

        if (strcmp(buf, "exit\n") == 0) break;

        pid_t pid = fork();
        if (pid == 0) {
            char *args[] = {buf, NULL};
            execvp(args[0], args);
            
            // 加上了问题1提到的 perror
            perror("Command not found");
            exit(1);
        } else {
            wait(NULL);
        }
    }
    return 0;
}
