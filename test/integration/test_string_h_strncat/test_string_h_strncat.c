#include <string.h>

int main(void) {
    char buf[16];
    strncpy(buf, "hello", 6);
    strncat(buf, " world", 6);
    char *p = strchr(buf, ' ');
    if (p == 0)
        return 1;
    return 0;
}
