#include <stdio.h>

/* Verifies that a # inside a block comment in a macro replacement
 * list does not trigger a "hash in object-like macro" error.
 * This matches the ARG_MAX pattern from system headers. */
#define LIMIT 4096   /* # entries allowed */
#define BUF   128    /* # bytes per entry */

int main(void) {
    printf("%d\n", LIMIT);
    printf("%d\n", BUF);
    return 0;
}
