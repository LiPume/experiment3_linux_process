/*
 * 文件名：pipe_main_practice.c
 * 作用：实验三——Linux 进程管理——管道通信（标准验收版）
 *
 * 功能要求（按教材实验要求实现）：
 * 1. 父进程先创建一个匿名管道
 * 2. 父进程再创建 3 个子进程
 * 3. 3 个子进程通过同一个管道向父进程发送消息
 * 4. 父进程等待 3 个子进程全部发送完成后，再统一接收消息
 * 5. 使用 POSIX 信号量，实现对子进程访问管道写端的互斥
 *
 * 适合用途：
 * - 课程实验验收
 * - 报告截图
 * - 答辩讲解
 */

#include <stdio.h>      // printf, perror
#include <stdlib.h>     // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h>     // pipe, fork, read, write, close, getpid, getppid
#include <string.h>     // memset, snprintf
#include <sys/wait.h>   // waitpid
#include <sys/types.h>  // pid_t
#include <fcntl.h>      // O_CREAT, O_EXCL
#include <semaphore.h>  // sem_open, sem_wait, sem_post, sem_close, sem_unlink
#include <errno.h>      // errno

/* 一共创建 3 个子进程 */
#define CHILD_NUM 3

/* 文本消息最大长度 */
#define TEXT_SIZE 128

/* 命名信号量名字：用于多个进程共享 */
#define SEM_NAME "/exp3_pipe_write_sem"

/*
 * 这个结构体表示“一条完整消息”
 *
 * 为什么用结构体？
 * 因为管道本质上是字节流，不天然保留“消息边界”。
 * 如果只写普通字符串，父进程读的时候可能遇到：
 * - 一次读到多条消息
 * - 一次只读到半条消息
 *
 * 这里用固定大小的结构体进行读写，更适合课程实验。
 * 每个子进程都写入一个固定大小的结构体，
 * 父进程也按同样大小读取，逻辑更清楚。
 */
typedef struct {
    int child_no;           // 第几个子进程（1 / 2 / 3）
    pid_t pid;              // 子进程自己的 PID
    pid_t ppid;             // 子进程的父进程 PID
    char text[TEXT_SIZE];   // 子进程想发送给父进程的文本
} PipeMessage;

/*
 * 功能：封装“子进程要做的工作”
 * 参数：
 *   write_fd  - 管道写端文件描述符
 *   child_no  - 当前是第几个子进程
 *   sem       - POSIX 命名信号量
 *
 * 子进程的逻辑：
 * 1. 组织一条消息
 * 2. 先申请信号量，进入临界区
 * 3. 向管道写入固定大小结构体
 * 4. 释放信号量
 * 5. 关闭写端并退出
 */
void child_process_work(int write_fd, int child_no, sem_t *sem) {
    PipeMessage msg;
    ssize_t write_bytes;

    /* 先把结构体清零，避免里面出现脏数据 */
    memset(&msg, 0, sizeof(msg));

    /* 填写结构体中的各个字段 */
    msg.child_no = child_no;
    msg.pid = getpid();
    msg.ppid = getppid();

    /* 组织要发送的文本 */
    snprintf(msg.text, sizeof(msg.text),
             "Hello parent, I am child %d. My pid = %d, my ppid = %d.",
             child_no, msg.pid, msg.ppid);

    printf("[Child %d | pid=%d] ready to send message.\n", child_no, getpid());

    /*
     * sem_wait：
     * 如果信号量当前值 > 0，就减 1，进入临界区；
     * 如果信号量当前值 = 0，就阻塞等待。
     *
     * 这里的作用：保证同一时刻只有一个子进程在写管道。
     */
    if (sem_wait(sem) == -1) {
        perror("sem_wait failed");
        close(write_fd);
        exit(EXIT_FAILURE);
    }

    printf("[Child %d | pid=%d] entered critical section, writing pipe...\n",
           child_no, getpid());

    /*
     * 向管道写入一整个结构体
     * 注意：这里写的是 sizeof(PipeMessage) 个字节，不是字符串长度。
     */
    write_bytes = write(write_fd, &msg, sizeof(msg));
    if (write_bytes == -1) {
        perror("child write failed");

        /* 发生错误后也要释放信号量，防止其他进程永远卡死 */
        sem_post(sem);
        close(write_fd);
        exit(EXIT_FAILURE);
    }

    printf("[Child %d | pid=%d] write finished, actual bytes = %zd.\n",
           child_no, getpid(), write_bytes);

    /* 写完后释放信号量，让其他子进程可以写 */
    if (sem_post(sem) == -1) {
        perror("sem_post failed");
        close(write_fd);
        exit(EXIT_FAILURE);
    }

    /* 子进程写完就关闭写端 */
    close(write_fd);

    /* 子进程结束 */
    exit(EXIT_SUCCESS);
}

int main() {
    int fd[2];                      // fd[0] = 读端，fd[1] = 写端
    pid_t child_pids[CHILD_NUM];    // 保存 3 个子进程的 PID，便于父进程 waitpid
    int i;
    int status;
    sem_t *sem;
    PipeMessage msg;
    ssize_t read_bytes;

    /*
     * 第一步：创建匿名管道
     * 必须在 fork() 之前创建！
     * 因为这样子进程才能继承到同一条管道的文件描述符。
     */
    if (pipe(fd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    /*
     * 第二步：创建命名 POSIX 信号量
     *
     * sem_unlink 先删除同名旧信号量，避免上次程序异常退出后残留。
     */
    sem_unlink(SEM_NAME);

    /*
     * sem_open 参数说明：
     * 1. SEM_NAME：信号量名字
     * 2. O_CREAT | O_EXCL：如果不存在就创建，并要求这次必须新建
     * 3. 0644：权限
     * 4. 1：初始值为 1，相当于“互斥锁”
     */
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        close(fd[0]);
        close(fd[1]);
        exit(EXIT_FAILURE);
    }

    printf("[Parent | pid=%d] pipe created successfully. read_fd=%d, write_fd=%d\n",
           getpid(), fd[0], fd[1]);

    /*
     * 第三步：循环创建 3 个子进程
     */
    for (i = 0; i < CHILD_NUM; i++) {
        child_pids[i] = fork();

        if (child_pids[i] < 0) {
            perror("fork failed");

            close(fd[0]);
            close(fd[1]);
            sem_close(sem);
            sem_unlink(SEM_NAME);
            exit(EXIT_FAILURE);
        }

        if (child_pids[i] == 0) {
            /*
             * 这里是子进程代码
             *
             * 子进程只负责“写”，不负责“读”，
             * 所以它应该关闭读端 fd[0]。
             */
            close(fd[0]);

            /* 调用封装好的子进程工作函数 */
            child_process_work(fd[1], i + 1, sem);
        }
    }

    /*
     * 这里回到父进程
     *
     * 父进程只负责“读”，不负责“写”，
     * 所以它应该关闭写端 fd[1]。
     *
     * 这一步很重要：
     * 如果父进程不关闭自己的写端，那么以后 read() 判断 EOF 时会受影响。
     */
    close(fd[1]);

    printf("[Parent | pid=%d] waiting for all children to finish...\n", getpid());

    /*
     * 第四步：父进程等待 3 个子进程全部结束
     * 这一步是为了满足教材要求：
     * “父进程等待三个子进程全部发完信息后再接收信息”
     */
    for (i = 0; i < CHILD_NUM; i++) {
        if (waitpid(child_pids[i], &status, 0) == -1) {
            perror("waitpid failed");
        }
    }

    printf("[Parent | pid=%d] all children finished. Now start reading pipe.\n",
           getpid());

    /*
     * 第五步：父进程统一读取消息
     *
     * 每次按 sizeof(PipeMessage) 大小读取一条固定消息
     */
    while ((read_bytes = read(fd[0], &msg, sizeof(msg))) > 0) {
        printf("------------------------------------------------------------\n");
        printf("[Parent] actual read bytes = %zd\n", read_bytes);
        printf("[Parent] received one message:\n");
        printf("         child_no = %d\n", msg.child_no);
        printf("         pid      = %d\n", msg.pid);
        printf("         ppid     = %d\n", msg.ppid);
        printf("         text     = %s\n", msg.text);
    }

    /*
     * read 返回值说明：
     * > 0 ：成功读取到字节
     * = 0 ：读到 EOF，说明所有写端都关闭，且数据已读完
     * = -1：出错
     */
    if (read_bytes == -1) {
        perror("parent read failed");
    } else if (read_bytes == 0) {
        printf("------------------------------------------------------------\n");
        printf("[Parent] pipe reached EOF. All messages have been read.\n");
    }

    /* 父进程关闭读端 */
    close(fd[0]);

    /* 关闭并删除信号量 */
    sem_close(sem);
    sem_unlink(SEM_NAME);

    return 0;
}