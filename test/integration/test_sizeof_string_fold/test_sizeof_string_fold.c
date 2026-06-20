#include <stdio.h>
int main(void) {
    /* sizeof("hi") == 3 (2 chars + null terminator) */
    int a = sizeof("hi");
    /* sizeof("") == 1 (null terminator only) */
    int b = sizeof("");
    printf("%d %d\n", a, b);
    return 0;
}
