#include <stdio.h>
int main(void) {
    int x = 5;
    int y = 0;
    int z = -3;
    int a = !!x;
    int b = !!y;
    int c = !!z;
    int d = !!0;
    int e = !!7;
    printf("%d %d %d %d %d\n", a, b, c, d, e);
    return 0;
}
