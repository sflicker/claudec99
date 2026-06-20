#include <stdio.h>
int main(void) {
    int a = 0;
    int b = 0;
    goto first;
    a = 99;    /* dead -- before first label */
    first:
    a = 1;     /* alive -- reached via goto */
    goto second;
    b = 99;    /* dead -- between labels */
    second:
    b = 2;     /* alive -- reached via goto */
    printf("%d %d\n", a, b);
    return 0;
}
