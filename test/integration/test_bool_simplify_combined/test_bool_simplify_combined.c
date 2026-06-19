#include <stdio.h>
int main(void) {
    int x = 4;
    int a = !!x && 1;
    int b = (x || 0) && 0;
    int c = !!x || 0;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
