#include <stdio.h>
int main(void) {
    int x = 5;
    int y = 0;
    int a = x || 0;
    int b = y || 0;
    printf("%d %d\n", a, b);
    return 0;
}
