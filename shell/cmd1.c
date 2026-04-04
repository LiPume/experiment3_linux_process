#include <stdio.h>
#include <unistd.h>

int main() {
    printf("===== cmd1: Process Information =====\n");
    printf("Current PID  : %d\n", getpid());
    printf("Parent PID   : %d\n", getppid());
    printf("This program is executed by myshell's child process.\n");
    return 0;
}