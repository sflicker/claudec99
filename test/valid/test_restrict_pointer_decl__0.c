#include <string.h>
int main(void) {
    char dst[8];
    const char *src = "hello";
    memmove(dst, src, 6);
    return strcmp(dst, "hello");
}
