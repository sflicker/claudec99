#include <stdio.h>
int main(void) {
    int a = 1 && 1;
    int b = 0 && 1;
    int c = 1 && 0;
    int d = 0 && 0;
    int e = 1 || 0;
    int f = 0 || 1;
    int g = 0 || 0;
    printf("%d %d %d %d %d %d %d\n", a, b, c, d, e, f, g);
    return 0;
}
