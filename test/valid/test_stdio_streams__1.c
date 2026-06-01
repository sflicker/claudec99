#include <stdio.h>

int main(void) {
    fprintf(stdout, "out\n");
    fprintf(stderr, "err\n");
    return stdin != 0 && stdout != 0 && stderr != 0;  /* expect 1 */
}
