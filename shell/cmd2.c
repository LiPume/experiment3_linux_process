#include <stdio.h>
#include <time.h>

int main() {
    time_t now;
    struct tm *t;

    time(&now);
    t = localtime(&now);

    printf("===== cmd2: Current Time =====\n");
    printf("Current local time: %04d-%02d-%02d %02d:%02d:%02d\n",
           t->tm_year + 1900,
           t->tm_mon + 1,
           t->tm_mday,
           t->tm_hour,
           t->tm_min,
           t->tm_sec);

    return 0;
}