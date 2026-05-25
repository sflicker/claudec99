#include <string.h>

int main(void) {
    char src[4] = "abc";
    char dst[4];
    memcpy(dst, src, 4);
    if (dst[0] == 'a' && dst[1] == 'b' && dst[2] == 'c' && dst[3] == 0)
        return 0;
    return 1;
}
