#include <stdio.h>
int main(void) {
    int x = 0;
    int n = 0;
    for (x = 5; 0; x++) { n = 99; }
    printf("%d %d\n", x, n);
    return 0;
}
