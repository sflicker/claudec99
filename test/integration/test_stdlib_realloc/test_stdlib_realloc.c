#include <stdlib.h>

int main(void) {
    int *p = malloc(sizeof(int));
    *p = 7;
    p = realloc(p, 4 * sizeof(int));
    p[1] = 11;
    p[2] = 13;
    p[3] = 17;
    int sum = p[0] + p[1] + p[2] + p[3];
    free(p);
    return sum;
}
