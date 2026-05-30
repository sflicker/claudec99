#include <stdarg.h>

int mixed(int ignored, ...) {
    va_list ap;
    int a;
    long b;
    unsigned long long c;

    va_start(ap, ignored);

    a = va_arg(ap, int);
    b = va_arg(ap, long);
    c = va_arg(ap, unsigned long long);

    va_end(ap);

    return a + b + c;
}

int main(void) {
    return mixed(0, 10, 20L, 12ULL);
}
