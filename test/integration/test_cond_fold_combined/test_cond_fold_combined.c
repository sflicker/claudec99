#include <stdio.h>
int main(void) {
    int a = (2 + 3) ? 10 : 20;
    int b = (1 - 1) ? 10 : 20;
    printf("%d %d\n", a, b);
    return 0;
}
