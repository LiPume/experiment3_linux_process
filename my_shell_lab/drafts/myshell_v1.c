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

        // 早期版本：直接读取，忘记清洗换行符
        if (fgets(buf, sizeof(buf), stdin) == NULL) break;
        
        if (strcmp(buf, "exit\n") == 0) break; 

        pid_t pid = fork();
        if (pid == 0) {
            char *args[] = {buf, NULL};
            execvp(args[0], args);
            // 自然的错误输出
            printf("Command not found: %s\n", buf);
            exit(1);
        } else {
            wait(NULL);
        }
    }
    return 0;
}
