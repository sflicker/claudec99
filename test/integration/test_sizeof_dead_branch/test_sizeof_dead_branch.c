#include <stdio.h>
int main(void) {
    int a = 0;
    /* sizeof(long) -> 8; 8 == 8 -> 1 (stage 143); if(1) keeps then-branch */
    if (sizeof(long) == 8) { a = 10; } else { a = 99; }
    int b = 0;
    /* sizeof(int) -> 4; 4 == 8 -> 0 (stage 143); if(0) keeps else-branch */
    if (sizeof(int) == 8) { b = 99; } else { b = 20; }
    printf("%d %d\n", a, b);
    return 0;
}
