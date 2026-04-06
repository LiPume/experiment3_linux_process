/*
 * 文件名：pipe_main.c
 * 模块：管道通信
 * 负责人：
 * 功能：父进程创建管道并与3个子进程通信
 * 关键系统调用：pipe(), fork(), read(), write(), close(), wait()
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
            close(fd[0]);

	    sleep(3);
	    
            sem_wait(write_sem);

            char msg[MSG_SIZE];
            snprintf(msg, sizeof(msg),
                     "来自子进程 %d (pid=%d) 的消息\n", i + 1, getpid());

            printf("子进程 %d：准备写入管道...\n", i + 1);
            write(fd[1], msg, strlen(msg));
            printf("子进程 %d：写入完成。\n", i + 1);

            sem_post(write_sem);

            close(fd[1]);
            exit(0);
        }
    }

    close(fd[1]);

    printf("父进程：现在开始读取管道，如果管道为空，read会阻塞...\n");

    int n = read(fd[0], buffer, sizeof(buffer));
    printf("父进程：read返回，读取到 %d 字节。\n", n);

    if (n > 0) {
	write(STDOUT_FILENO, "父进程收到：", strlen("父进程收到："));
	write(STDOUT_FILENO, buffer, n);
    }

    for (int i = 0; i < CHILD_NUM; i++) {
	wait(NULL);
    }

    close(fd[0]);

    sem_close(write_sem);
    sem_unlink("/pipe_write_sem");

    return 0;
}
