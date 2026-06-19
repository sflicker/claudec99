#include <stdio.h>
int main(void) {
    int x = 5;
    int y = 0;
    int a = x && 1;
    int b = y && 1;
    int c = x && 3;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
