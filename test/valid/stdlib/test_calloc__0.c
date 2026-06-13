#include <stdlib.h>

int main(void) {
    int *p;

    p = calloc(4, sizeof(int));
    if (!p) {
        return 1;
    }

    if (p[0] != 0) {
        free(p);
        return 2;
    }

    if (p[1] != 0) {
        free(p);
        return 3;
    }

    p[2] = 42;

    if (p[2] != 42) {
        free(p);
        return 4;
    }

    free(p);
    return 0;
}
