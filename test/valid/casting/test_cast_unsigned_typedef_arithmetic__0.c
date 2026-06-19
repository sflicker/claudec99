#include <stddef.h>

/* (size_t)0 - (size_t)1 wraps to ULONG_MAX under unsigned arithmetic,
   which is > 0 — verify the cast preserves unsigned semantics. */
int main(void) {
    if (!((size_t)0 - (size_t)1 > 0)) return 1;

    size_t a = 3;
    size_t b = 5;
    if (!((a - b) > 0)) return 2;

    if (!((size_t)1 - (size_t)2 > (size_t)0)) return 3;

    return 0;
}
