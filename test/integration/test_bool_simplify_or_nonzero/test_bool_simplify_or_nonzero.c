#include <stdio.h>
int main(void) {
    int x = 0;
    int a = x || 1;
    int b = x || 5;
    int c = 1 || 1;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
