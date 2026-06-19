#include <stdio.h>
int main(void) {
    int a = !0;
    int b = !1;
    int c = !42;
    int d = !(-1);
    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
