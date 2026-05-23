#include <stdio.h>

#define LIMIT 3

int main(void) {
    int i = 0;
    do {
        i = i + 1;
    } while (i < LIMIT);
    printf("%d\n", i);
    return 0;
}
