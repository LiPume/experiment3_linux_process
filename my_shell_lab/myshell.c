#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char buf[100];
    
    while (1) {
        // 1. 打印提示符
        printf("HDU-Computer-Science-Shell > "); 
        fflush(stdout); // 强制刷新缓冲区，确保提示符先显示出来

        // 2. 获取用户输入
        if (fgets(buf, sizeof(buf), stdin) == NULL) break;
        
        // 去掉用户输入末尾的换行符 \n
        buf[strcspn(buf, "\n")] = 0;

        // 3. 判断是否退出
        if (strcmp(buf, "exit") == 0) {
            printf("正在退出 Shell... 再见！\n");
            break;
        }

        // 4. 判断命令是否为空（直接按回车的情况）
        if (strlen(buf) == 0) continue;

        // 5. 创建子进程 (fork)
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork 出错了");
        } 
        else if (pid == 0) {
            // --- 这里是子进程 ---
            // 构造路径，比如输入 "cmd1" 变成 "./cmd1"
            char path[120] = "./";
            strcat(path, buf);

            // 执行程序 (execl)
            if (execl(path, buf, NULL) == -1) {
                // 如果 execl 返回了，说明执行失败（找不到文件）
                printf("错误：找不到命令 '%s'，请确保输入的是 cmd1, cmd2 或 cmd3。\n", buf);
                exit(1); 
            }
        } 
        else {
            // --- 这里是父进程 ---
            // 父进程必须等待子进程运行完，不然会乱套
            wait(NULL); 
        }
    }
    return 0;
}
