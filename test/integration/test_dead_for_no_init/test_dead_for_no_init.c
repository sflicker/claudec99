#include <stdio.h>
int main(void) {
    int n = 0;
    for (; 0; ) { n = 99; }
    printf("%d\n", n);
    return 0;
}
