#include <stdio.h>
int main(void) {
    int a = 0;
    if (1) { a = 10; } else { a = 99; }
    int b = 0;
    if (-3) { b = 20; } else { b = 99; }
    printf("%d %d\n", a, b);
    return 0;
}
