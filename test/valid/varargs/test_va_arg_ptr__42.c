#include <stdarg.h>

int read_ptr(int ignored, ...) {
    va_list ap;
    int *p;

    va_start(ap, ignored);
    p = va_arg(ap, int *);
    va_end(ap);

    return *p;
}

int main(void) {
    int x;
    x = 42;
    return read_ptr(0, &x);
}
