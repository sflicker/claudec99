#include <stdio.h>
int main(void) {
    int x = 99;
    int a = x * 0;
    int b = 0 * x;
    int c = 0 / x;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
