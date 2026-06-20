#include <stdio.h>
int main(void) {
    int x = 5;
    int y = -3;
    int a = -(-x);        /* non-zero positive var -> 5 */
    int b = -(-y);        /* negative var -> -3 */
    int c = -(-0);        /* const: stage-144 handles -> 0 */
    int d = -(-7);        /* const: stage-144 handles -> 7 */
    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
