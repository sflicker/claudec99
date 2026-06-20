#include <stdio.h>
int main(void) {
    int a = 0 ? 42 : 99;
    int b = 0 ? 10 : 20;
    printf("%d %d\n", a, b);
    return 0;
}
