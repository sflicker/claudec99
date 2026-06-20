#include <stdio.h>
struct Point { int x; int y; };
int main(void) {
    /* sizeof(struct Point) -> 8 (two 4-byte ints, no padding) */
    int a = sizeof(struct Point);
    /* sizeof(int[4]) -> 16 */
    int b = sizeof(int[4]);
    printf("%d %d\n", a, b);
    return 0;
}
