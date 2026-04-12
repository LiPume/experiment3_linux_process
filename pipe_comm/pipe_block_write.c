#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define WRITE_SIZE 4096

int main() {
    int fd[2];
    pid_t pid;

    if (pipe(fd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

#ifdef F_GETPIPE_SZ
    int pipe_size = fcntl(fd[0], F_GETPIPE_SZ);
    if (pipe_size != -1) {
        printf("父进程：当前管道默认大小 = %d 字节\n", pipe_size);
    }
#endif

    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(fd[0]);

        char buf[WRITE_SIZE];
        memset(buf, 'A', sizeof(buf));

        int count = 0;
        while (1) {
            count++;
            printf("子进程：准备进行第 %d 次写入...\n", count);
            write(fd[1], buf, sizeof(buf));
            printf("子进程：第 %d 次写入完成，写入 %ld 字节\n", count, sizeof(buf));
        }

        close(fd[1]);
        exit(0);
    } else {
        close(fd[1]);

        printf("父进程：先睡眠15秒，不读取管道，观察子进程是否会阻塞...\n");
        sleep(15);

        printf("父进程：开始读取管道数据，释放管道空间...\n");

        char read_buf[WRITE_SIZE];
        int n;
        while ((n = read(fd[0], read_buf, sizeof(read_buf))) > 0) {
            printf("父进程：读取了 %d 字节\n", n);
            sleep(1);
        }

        close(fd[0]);
        wait(NULL);
    }

    return 0;
}
