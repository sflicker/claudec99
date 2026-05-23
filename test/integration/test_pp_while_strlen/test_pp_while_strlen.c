#include <string.h>

int my_strlen(const char *s) {
    int n = 0;
    while (*s) {
        n = n + 1;
        s = s + 1;
    }
    return n;
}

int main(void) {
    return my_strlen("hello") - (int)strlen("hello");
}
