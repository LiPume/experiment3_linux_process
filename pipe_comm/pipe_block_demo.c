/*
 * 文件名：pipe_block_demo_practice.c
 * 作用：实验三——Linux 进程管理——管道通信（阻塞观察版）
 *
 * 本程序专门用于观察以下两个现象：
 * 1. 管道为空时，read() 会阻塞
 * 2. 管道满时，write() 会阻塞
 *
 * 这不是“标准验收版主程序”，
 * 而是为了帮助理解管道阻塞型读写机制而写的观察程序。
 *
 * 程序分为两个演示：
 * Demo 1：空管道读阻塞
 * Demo 2：满管道写阻塞
 */

#define _GNU_SOURCE

#include <stdio.h>      // printf, perror
#include <stdlib.h>     // exit
#include <unistd.h>     // pipe, fork, read, write, close, sleep, usleep, getpid
#include <string.h>     // memset, strlen
#include <sys/wait.h>   // waitpid
#include <sys/types.h>  // pid_t
#include <fcntl.h>      // fcntl, F_GETPIPE_SZ
#include <sys/time.h>   // gettimeofday

/*
 * 功能：获取当前时间（单位：毫秒）
 * 作用：方便我们统计一次 read/write 调用了多久
 */
long long now_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
}

/*
 * Demo 1：演示“管道为空时，read() 阻塞”
 *
 * 设计思路：
 * - 父进程创建一个管道
 * - fork 一个子进程作为“读者”
 * - 子进程立刻去 read()
 * - 但父进程先不写，故意 sleep(3)
 * - 因为这 3 秒里管道是空的，所以子进程会在 read() 处阻塞
 * - 3 秒后父进程再写入数据，子进程被唤醒
 */
void demo_read_block_when_empty() {
    int fd[2];
    pid_t pid;
    char buf[64];
    ssize_t n;
    long long t1, t2;

    printf("\n==================== Demo 1：管道为空时，read() 阻塞 ====================\n");

    if (pipe(fd) == -1) {
        perror("pipe failed");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        /*
         * 子进程：读者
         * 它只需要读，所以关闭写端
         */
        close(fd[1]);

        memset(buf, 0, sizeof(buf));

        printf("[Reader child | pid=%d] about to call read().\n", getpid());
        printf("[Reader child | pid=%d] if pipe is empty, read() will block...\n", getpid());

        t1 = now_ms();
        n = read(fd[0], buf, sizeof(buf) - 1);
        t2 = now_ms();

        if (n == -1) {
            perror("read failed");
        } else if (n == 0) {
            printf("[Reader child | pid=%d] read returned 0 (EOF).\n", getpid());
        } else {
            buf[n] = '\0';
            printf("[Reader child | pid=%d] read returned after %lld ms.\n",
                   getpid(), t2 - t1);
            printf("[Reader child | pid=%d] actual read bytes = %zd, data = \"%s\"\n",
                   getpid(), n, buf);
        }

        close(fd[0]);
        exit(0);
    } else {
        /*
         * 父进程：写者
         * 它只需要写，所以关闭读端
         */
        close(fd[0]);

        /*
         * 故意延迟 3 秒不写
         * 这样子进程读时会因为管道为空而阻塞
         */
        sleep(3);

        const char *msg = "wake up reader";
        n = write(fd[1], msg, strlen(msg));
        if (n == -1) {
            perror("write failed");
        } else {
            printf("[Parent | pid=%d] after 3 seconds, wrote %zd bytes to pipe.\n",
                   getpid(), n);
        }

        close(fd[1]);
        waitpid(pid, NULL, 0);
    }
}

/*
 * Demo 2：演示“管道满时，write() 阻塞”
 *
 * 设计思路：
 * - 父进程创建一个管道
 * - fork 出一个“慢速读者”
 * - 读者先 sleep(5)，故意不读
 * - 再 fork 出一个“持续写者”
 * - 写者不停向管道写 4096 字节数据块
 * - 当管道被写满后，某次 write() 会明显耗时增加
 * - 5 秒后读者开始读，写者被唤醒继续写
 */
void demo_write_block_when_full() {
    int fd[2];
    pid_t reader_pid, writer_pid;
    int pipe_size;
    char buf[4096];
    ssize_t n;
    long long t1, t2;
    int total_written = 0;
    int total_read = 0;
    int i;

    printf("\n==================== Demo 2：管道满时，write() 阻塞 ====================\n");

    if (pipe(fd) == -1) {
        perror("pipe failed");
        exit(1);
    }

    /*
     * F_GETPIPE_SZ：读取当前管道缓冲区大小
     * 在 Linux 下常见默认值可能是 65536，也可能因系统而异。
     */
    pipe_size = fcntl(fd[1], F_GETPIPE_SZ);
    if (pipe_size == -1) {
        perror("fcntl(F_GETPIPE_SZ) failed");
        pipe_size = -1;
    } else {
        printf("[Parent] default pipe capacity = %d bytes\n", pipe_size);
    }

    /*
     * 先创建慢速读者
     */
    reader_pid = fork();
    if (reader_pid < 0) {
        perror("fork reader failed");
        exit(1);
    }

    if (reader_pid == 0) {
        /*
         * 子进程1：慢速读者
         * 它只读，所以关闭写端
         */
        close(fd[1]);

        printf("[Slow reader | pid=%d] sleep 5 seconds first.\n", getpid());
        printf("[Slow reader | pid=%d] during this time, writer may block when pipe gets full.\n",
               getpid());

        sleep(5);

        while ((n = read(fd[0], buf, sizeof(buf))) > 0) {
            total_read += n;
            printf("[Slow reader | pid=%d] read %zd bytes, total_read = %d\n",
                   getpid(), n, total_read);

            /*
             * 故意读得慢一点
             * 这样更容易观察“写端刚被唤醒，又很快可能再次被卡住”的现象
             */
            usleep(200000);  // 200ms
        }

        if (n == 0) {
            printf("[Slow reader | pid=%d] reached EOF.\n", getpid());
        } else if (n == -1) {
            perror("slow reader read failed");
        }

        close(fd[0]);
        exit(0);
    }

    /*
     * 再创建持续写者
     */
    writer_pid = fork();
    if (writer_pid < 0) {
        perror("fork writer failed");
        exit(1);
    }

    if (writer_pid == 0) {
        /*
         * 子进程2：持续写者
         * 它只写，所以关闭读端
         */
        close(fd[0]);

        /* 用字符 'A' 填满缓冲区 */
        memset(buf, 'A', sizeof(buf));

        /*
         * 这里不写无限循环，而是写固定 40 次。
         * 这样程序更容易结束，也更适合实验演示和截图。
         */
        for (i = 1; i <= 40; i++) {
            t1 = now_ms();
            n = write(fd[1], buf, sizeof(buf));
            t2 = now_ms();

            if (n == -1) {
                perror("writer write failed");
                break;
            }

            total_written += n;

            printf("[Writer child | pid=%d] write #%d: n = %zd, total_written = %d, cost = %lld ms\n",
                   getpid(), i, n, total_written, t2 - t1);

            /*
             * 关键观察点：
             * 如果某次 write() 原本都只要 0~1 ms，
             * 突然变成几千毫秒，
             * 通常说明它因为“管道满了”而被阻塞过。
             */
        }

        printf("[Writer child | pid=%d] writing finished, now close write end.\n", getpid());

        close(fd[1]);
        exit(0);
    }

    /*
     * 父进程自己既不读也不写，所以两个端都关闭
     * 让两个子进程独立完成“慢读者”和“持续写者”的实验
     */
    close(fd[0]);
    close(fd[1]);

    waitpid(reader_pid, NULL, 0);
    waitpid(writer_pid, NULL, 0);
}

int main() {
    demo_read_block_when_empty();
    demo_write_block_when_full();
    return 0;
}