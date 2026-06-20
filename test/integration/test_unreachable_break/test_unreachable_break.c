#include <stdio.h>
int main(void) {
    int n = 0;
    while (1) {
        n = 5;
        break;
        n = 99; /* dead */
    }
    printf("%d\n", n);
    return 0;
}
