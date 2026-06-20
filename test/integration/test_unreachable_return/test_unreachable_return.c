#include <stdio.h>
int foo(void) {
    return 1;
    return 2; /* dead */
    return 3; /* dead */
}
int main(void) {
    printf("%d\n", foo());
    return 0;
}
