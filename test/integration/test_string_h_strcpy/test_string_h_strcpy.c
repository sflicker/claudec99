#include <string.h>

int main(void) {
    char dst[8];
    strcpy(dst, "hello");
    if (dst[0] == 'h' && dst[1] == 'e' && dst[2] == 'l' &&
        dst[3] == 'l' && dst[4] == 'o' && dst[5] == 0)
        return 0;
    return 1;
}
