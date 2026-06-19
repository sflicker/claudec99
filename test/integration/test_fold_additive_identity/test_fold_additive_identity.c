#include <stdio.h>
int main(void) {
    int x = 42;
    int a = x + 0;
    int b = 0 + x;
    int c = x - 0;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
