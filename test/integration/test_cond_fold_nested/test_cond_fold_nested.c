#include <stdio.h>
int main(void) {
    int a = 1 ? 1 : (0 ? 2 : 3);
    int b = 0 ? (1 ? 2 : 3) : 4;
    printf("%d %d\n", a, b);
    return 0;
}
