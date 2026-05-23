#include <stdio.h>

#define USE_ADD 1

int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }

int main(void) {
    int (*op)(int, int);
#if USE_ADD
    op = add;
#else
    op = sub;
#endif
    printf("%d\n", op(10, 3));
    return 0;
}
