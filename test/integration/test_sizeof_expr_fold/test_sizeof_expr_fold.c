#include <stdio.h>
int main(void) {
    /* sizeof(int) * 8 -> 4 * 8 (stage 151 then stage 146 strength reduction) */
    int a = sizeof(int) * 8;
    /* sizeof(long) + sizeof(int) -> 8 + 4 -> 12 (stage 151 then stage 143) */
    int b = (int)sizeof(long) + (int)sizeof(int);
    printf("%d %d\n", a, b);
    return 0;
}
