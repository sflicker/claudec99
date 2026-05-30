#include <stdarg.h>

int check(int ignored, ...) {
    va_list ap;
    long long x;

    va_start(ap, ignored);
    x = va_arg(ap, long long);
    va_end(ap);

    return x == 42LL;
}

int main(void) {
    return check(0, 42LL);
}
