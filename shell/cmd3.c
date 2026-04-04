#include <stdio.h>

int main() {
    int i, sum = 0;

    printf("===== cmd3: Sum Calculation =====\n");
    for (i = 1; i <= 10; i++) {
        sum += i;
        printf("i = %d, current sum = %d\n", i, sum);
    }

    printf("Final result: sum(1~10) = %d\n", sum);
    return 0;
}