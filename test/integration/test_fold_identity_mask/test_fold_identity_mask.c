#include <stdio.h>
int main(void) {
    int x = 0xAB;
    int a = x & ~0;
    printf("%d\n", a);
    return 0;
}
