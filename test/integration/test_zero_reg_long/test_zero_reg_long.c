#include <stdio.h>
long lzero(void) { return 0L; }
int main(void) {
    printf("%ld\n", lzero());
    return 0;
}
