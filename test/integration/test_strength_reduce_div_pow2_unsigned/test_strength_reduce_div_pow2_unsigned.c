#include <stdio.h>
int main(void) {
    unsigned int x = 48;
    unsigned int a = x / 2;
    unsigned int b = x / 4;
    unsigned int c = x / 8;
    printf("%u %u %u\n", a, b, c);
    return 0;
}
