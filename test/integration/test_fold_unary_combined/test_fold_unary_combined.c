#include <stdio.h>
int main(void) {
    int a = -(3 + 4);
    int b = !(2 - 2);
    int c = ~(1 << 3);
    printf("%d %d %d\n", a, b, c);
    return 0;
}
