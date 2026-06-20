#include <stdio.h>
int main(void) {
    const int x = 5;
    int a = x;
    {
        const int x = 10;
        int b = x;
        printf("%d\n", b);
    }
    int c = x;
    printf("%d %d\n", a, c);
    return 0;
}
