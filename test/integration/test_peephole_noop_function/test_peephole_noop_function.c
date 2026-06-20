#include <stdio.h>
int add(int x, int y) { return x + y; }
int mul(int x, int y) { return x * y; }
int main(void) {
    printf("%d %d\n", add(3, 4), mul(3, 4));
    return 0;
}
