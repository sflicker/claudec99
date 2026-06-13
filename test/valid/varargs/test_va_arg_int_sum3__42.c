#include <stdarg.h>

int sum3(int first, ...) {
    va_list ap;
    int total;

    total = first;
    va_start(ap, first);

    total += va_arg(ap, int);
    total += va_arg(ap, int);
    va_end(ap);

    return total;
}

int main(void) {
    return sum3(10, 20, 12);
}
