#include <stddef.h>

/* Arithmetic through a size_t local variable preserves unsigned semantics. */
int main(void) {
    size_t big = (size_t)0 - (size_t)1;
    if (!(big > 0)) return 1;

    size_t x = 2;
    size_t y = 5;
    size_t diff = x - y;
    if (!(diff > x)) return 2;

    return 0;
}
