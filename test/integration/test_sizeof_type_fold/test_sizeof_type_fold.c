#include <stdio.h>
int main(void) {
    /* sizeof(int) -> 4; 4 * 2 -> 8 (stage 143) */
    int a = sizeof(int) * 2;
    /* sizeof(long) -> 8; sizeof(char) -> 1; 8 - 1 -> 7 (stage 143) */
    int b = sizeof(long) - sizeof(char);
    /* sizeof(void *) -> 8; 8 / 2 -> 4 (stage 143) */
    int c = sizeof(void *) / 2;
    printf("%d %d %d\n", a, b, c);
    return 0;
}
