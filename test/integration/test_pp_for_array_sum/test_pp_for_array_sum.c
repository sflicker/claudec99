#include <stdio.h>

#define N 5

int main(void) {
    int a[N];
    int i;
    for (i = 0; i < N; i = i + 1) {
        a[i] = i + 1;
    }
    int sum = 0;
    for (i = 0; i < N; i = i + 1) {
        sum += a[i];
    }
    printf("%d\n", sum);
    return 0;
}
