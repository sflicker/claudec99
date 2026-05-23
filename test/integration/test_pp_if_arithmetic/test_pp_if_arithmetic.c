#include <stdio.h>

#define BASE 10
#define SCALE 3

#if BASE * SCALE > 25
#define MSG "big"
#else
#define MSG "small"
#endif

int main(void) {
    printf("%s\n", MSG);
    return 0;
}
