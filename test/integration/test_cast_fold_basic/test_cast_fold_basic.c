#include <stdio.h>
int main(void) {
    int  a = (int)42L;       /* (int)42L -> 42 (int) */
    long b = (long)7;        /* (long)7  -> 7L (long) */
    int  c = (int)100LL;     /* (int)100LL -> 100 (int) */
    printf("%d %ld %d\n", a, b, c);
    return 0;
}
