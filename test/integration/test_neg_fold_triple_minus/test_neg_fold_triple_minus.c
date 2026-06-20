#include <stdio.h>
int main(void) {
    int x = 4;
    int a = -(-(-x));     /* inner -(-x) -> x; then -(x) stays */
    printf("%d\n", a);
    return 0;
}
