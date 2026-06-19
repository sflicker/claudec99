#include <stdio.h>
int main(void) {
    int x = 5;
    int a = x - x;
    int b = x ^ x;
    int c = x & 0;
    int d = x | 0;
    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
