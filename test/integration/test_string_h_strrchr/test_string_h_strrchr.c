#include <string.h>

int main(void) {
    char *s = "hello";
    char *p = strrchr(s, 'l');
    if (p == 0)
        return 1;
    if (p - s == 3)
        return 0;
    return 2;
}
