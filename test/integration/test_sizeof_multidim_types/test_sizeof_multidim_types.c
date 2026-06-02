#include <stdio.h>

int main() {
    printf("%ld ", sizeof(int[1]));
    printf("%ld ", sizeof(int[4]));
    printf("%ld ", sizeof(int[2][2]));
    printf("%ld\n", sizeof(int[4][8]));
    return 0;
}
