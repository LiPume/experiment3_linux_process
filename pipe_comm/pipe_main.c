/*
 * 文件名：pipe_main.c
 * 模块：管道通信
 * 功能：父进程创建管道并与3个子进程通信
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>

#define CHILD_NUM 3
#define MSG_SIZE 128

int main() {
    int fd[2];
    pid_t pid;
    char buffer[MSG_SIZE];
    sem_t *write_sem;

    if (pipe(fd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    sem_unlink("/pipe_write_sem");
    write_sem = sem_open("/pipe_write_sem", O_CREAT, 0644, 1);
    if (write_sem == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    printf("父进程：管道创建成功。\n");

#ifdef F_GETPIPE_SZ
    int pipe_size = fcntl(fd[0], F_GETPIPE_SZ);
    if (pipe_size != -1) {
        printf("父进程：当前管道默认大小 = %d 字节\n", pipe_size);
    } else {
        perror("fcntl failed");
    }
#endif

    for (int i = 0; i < CHILD_NUM; i++) {
        pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            close(fd[0]);   // 子进程不用读端

            sleep(1);       // 只是为了更明显看到多个子进程运行

            sem_wait(write_sem);

            char msg[MSG_SIZE];
            snprintf(msg, sizeof(msg),
                     "来自子进程 %d (pid=%d) 的消息\n", i + 1, getpid());

            printf("子进程 %d：准备写入管道...\n", i + 1);
            int len = strlen(msg);
            int n = write(fd[1], msg, len);
            printf("子进程 %d：写入完成，实际写入 %d 字节。\n", i + 1, n);

            sem_post(write_sem);

            close(fd[1]);
            sem_close(write_sem);
            exit(0);
        }
    }

    close(fd[1]);   // 父进程不用写端

    printf("父进程：先等待3个子进程全部发送完消息...\n");
    for (int i = 0; i < CHILD_NUM; i++) {
        wait(NULL);
    }
    printf("父进程：3个子进程均已结束，开始统一读取管道内容。\n");

    int n;
    while ((n = read(fd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        printf("父进程：本次 read 返回 %d 字节。\n", n);
        printf("父进程收到：%s", buffer);
    }

    if (n == 0) {
        printf("父进程：管道已读完，read 返回 0。\n");
    } else if (n < 0) {
        perror("read failed");
    }

    close(fd[0]);
    sem_close(write_sem);
    sem_unlink("/pipe_write_sem");

    return 0;
}
