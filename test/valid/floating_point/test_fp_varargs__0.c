#include <stdarg.h>
static double sum(int n, ...) {
    va_list ap;
    va_start(ap, n);
    double s = 0.0;
    int i;
    for (i = 0; i < n; i++)
        s = s + va_arg(ap, double);
    va_end(ap);
    return s;
}
int main(void) {
    double r = sum(3, 1.0, 2.0, 3.0);
    return (int)r - 6;
}
