#include <stdio.h>
int main(void) {
    int x = 7;
    int a = x * (1 << 3);
    int b = x * (2 + 2);
    printf("%d %d\n", a, b);
    return 0;
}
