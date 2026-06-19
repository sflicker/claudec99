#include <stdio.h>
int main(void) {
    int x = 7;
    int a = x && 0;
    int b = 0 && 0;
    printf("%d %d\n", a, b);
    return 0;
}
