#include <stdio.h>

/* Verifies that block comments in #elif conditions are ignored. */
#define LEVEL 2

#if LEVEL == 1 /* first option */
#define MSG "one"
#elif LEVEL == 2 /* second option (this branch should be taken) */
#define MSG "two"
#else /* fallback */
#define MSG "other"
#endif

int main(void) {
    printf("%s\n", MSG);
    return 0;
}
