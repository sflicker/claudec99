#include <stdarg.h>

int pick7(int fixed, ...) {
    va_list ap;
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
    int g;

    va_start(ap, fixed);

    a = va_arg(ap, int);
    b = va_arg(ap, int);
    c = va_arg(ap, int);
    d = va_arg(ap, int);
    e = va_arg(ap, int);
    f = va_arg(ap, int);
    g = va_arg(ap, int);

    va_end(ap);

    return g;
}

int main(void) {
    return pick7(0, 1, 2, 3, 4, 5, 6, 42);
}
