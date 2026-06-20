#include <stdio.h>
int main(void) {
    int x = 3;
    int a = -(-x) + 0;    /* -(-x) -> x; x + 0 -> x */
    int b = -(-x) * 1;    /* -(-x) -> x; x * 1 -> x */
    int c = -(-x) - x;    /* -(-x) -> x; x - x -> 0 */
    printf("%d %d %d\n", a, b, c);
    return 0;
}
