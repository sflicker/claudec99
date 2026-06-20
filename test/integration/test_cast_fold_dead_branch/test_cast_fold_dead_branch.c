#include <stdio.h>
int main(void) {
    int a = 0;
    /* (int)sizeof(long)==8 -> 8==8 -> 1; if(1) keeps then-branch */
    if ((int)sizeof(long) == 8) { a = 1; } else { a = 99; }
    int b = 0;
    /* (int)sizeof(int)==8 -> 4==8 -> 0; if(0) keeps else-branch */
    if ((int)sizeof(int) == 8) { b = 99; } else { b = 2; }
    printf("%d %d\n", a, b);
    return 0;
}
