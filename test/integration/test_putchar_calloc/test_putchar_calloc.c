#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char *p;

    p = calloc(3, sizeof(char));
    if (!p) {
        return 1;
    }

    p[0] = 'O';
    p[1] = 'K';

    putchar(p[0]);
    putchar(p[1]);
    putchar('\n');

    free(p);
    return 0;
}
