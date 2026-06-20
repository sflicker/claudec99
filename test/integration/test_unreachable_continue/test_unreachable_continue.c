#include <stdio.h>
int main(void) {
    int n = 0;
    int i;
    for (i = 0; i < 3; i++) {
        n++;
        continue;
        n = 99; /* dead */
    }
    printf("%d\n", n);
    return 0;
}
