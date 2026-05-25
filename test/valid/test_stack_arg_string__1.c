#include <string.h>

int check(int a, int b, int c, int d, int e, int f, const char *s) {
    return strcmp(s, "hello") == 0;
}

int main(void) {
    return check(1, 2, 3, 4, 5, 6, "hello");
}
