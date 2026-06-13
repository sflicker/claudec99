#include <stdarg.h>
#include <string.h>

int check(int ignored, ...) {
    va_list ap;
    const char *s;

    va_start(ap, ignored);
    s = va_arg(ap, const char *);
    va_end(ap);

    return strcmp(s, "hello") == 0;
}

int main(void) {
    return check(0, "hello");
}
