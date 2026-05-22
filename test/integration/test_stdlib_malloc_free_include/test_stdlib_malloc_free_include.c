#include <stdlib.h>

int main(void) {
    int *p;
    p = malloc(sizeof(int));
    *p = 42;
    int value = *p;
    free(p);
    return value;
}
