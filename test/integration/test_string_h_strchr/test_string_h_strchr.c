#include <string.h>

int main(void) {
    char *s = "hello";
    char *p = strchr(s, 'e');
    if (p == 0)
        return 255;
    return p - s;
}
