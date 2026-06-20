#include <stdio.h>
int main(void) {
    int a = 1;
    goto skip;
    a = 99; /* dead */
    a = 88; /* dead */
    skip:
    printf("%d\n", a);
    return 0;
}
