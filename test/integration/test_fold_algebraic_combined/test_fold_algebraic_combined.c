#include <stdio.h>
int main(void) {
    int x = 3;
    int a = (x + 0) * 1;
    int b = (x * 0) + (x * 1);
    int c = (x - x) | (1 * x);
    printf("%d %d %d\n", a, b, c);
    return 0;
}
