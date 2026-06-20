#include <stdio.h>
int main(void) {
    int a = 1 ? 42 : 99;
    int b = 7 ? 10 : 20;
    int c = -1 ? 5 : 6;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
