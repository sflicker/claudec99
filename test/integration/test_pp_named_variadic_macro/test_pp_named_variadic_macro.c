#include <stdio.h>

/* GNU named variadic macro parameter: an identifier immediately followed by
 * "..." (no fixed parameters), as used by /usr/include/linux/stddef.h's
 * __struct_group(TAG, NAME, ATTRS, MEMBERS...). */
#define REPEAT_TWICE(MEMBERS...) MEMBERS; MEMBERS

int main(void) {
    int total = 0;
    REPEAT_TWICE(total++);
    printf("%d\n", total);
    return 0;
}
