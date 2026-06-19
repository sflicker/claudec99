#include <stdio.h>
int main(void) {
    int a = 0xF0 & 0xFF;
    int b = 0x0F | 0xF0;
    int c = 0xFF ^ 0x0F;
    int d = 1 << 5;
    int e = 128 >> 2;
    int f = ~0;
    printf("%d %d %d %d %d %d\n", a, b, c, d, e, f);
    return 0;
}
