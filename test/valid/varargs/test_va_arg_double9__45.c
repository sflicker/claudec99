/* CC99-002: va_arg double beyond the 8th XMM slot */
#include <stdarg.h>

int sumd(int n, ...) {
    va_list ap;
    int i;
    double s = 0.0;
    va_start(ap, n);
    for (i = 0; i < n; i = i + 1) {
        s = s + va_arg(ap, double);
    }
    va_end(ap);
    return (int)s;
}

int main(void) {
    return sumd(9, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
}
