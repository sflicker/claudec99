#include <stdio.h>

/* Verifies that block comments in #if conditions are ignored. */
#define FLAG 1

#if defined(FLAG) /* check if FLAG is set */
#define RESULT 10
#else
#define RESULT 20
#endif

#if FLAG == 1 /* expected branch */
#define BRANCH "true"
#else
#define BRANCH "false"
#endif

int main(void) {
    printf("%d\n", RESULT);
    printf("%s\n", BRANCH);
    return 0;
}
