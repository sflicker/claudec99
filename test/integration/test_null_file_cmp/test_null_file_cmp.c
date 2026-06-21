/* Stage 161: FILE * compared to NULL must work when NULL expands to (void *)0
 * (as defined by GCC system headers). */
#include <stdio.h>

int main(void) {
    FILE *fp = NULL;
    if (fp == NULL) {
        printf("FP is NULL\n");
    }
    printf("Hello\n");
    return 0;
}
