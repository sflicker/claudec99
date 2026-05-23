#include <stdio.h>

#define CHOICE 2

int main(void) {
    switch (CHOICE) {
        case 1: puts("one"); break;
        case 2: puts("two"); break;
        case 3: puts("three"); break;
        default: puts("other"); break;
    }
    return 0;
}
