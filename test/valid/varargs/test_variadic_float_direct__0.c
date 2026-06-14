#include <stdio.h>
#include <stdarg.h>
static double sum2(int n, ...) {
    va_list ap;
    va_start(ap, n);
    double a = va_arg(ap, double);
    double b = va_arg(ap, double);
    va_end(ap);
    return a + b;
}
int main(void) {
    float x = 1.5f, y = 2.5f;
    printf("%.1f\n", sum2(2, x, y));
    return 0;
}
