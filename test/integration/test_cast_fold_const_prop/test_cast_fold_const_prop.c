#include <stdio.h>
int main(void) {
    const long N = 100;
    int x = (int)N;          /* N->100L (stage 152); (int)100L->100 (stage 153) */
    int y = (int)N + 5;      /* -> 100 + 5 -> 105 (stage 143) */
    printf("%d %d\n", x, y);
    return 0;
}
