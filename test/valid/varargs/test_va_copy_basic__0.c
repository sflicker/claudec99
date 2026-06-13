#include <stdarg.h>
#include <stdio.h>

static int sum(int n, ...) {
    va_list ap, cp;
    va_start(ap, n);
    va_copy(cp, ap);
    int s1 = 0, s2 = 0;
    int i;
    for (i = 0; i < n; i++) s1 += va_arg(ap, int);
    for (i = 0; i < n; i++) s2 += va_arg(cp, int);
    va_end(ap);
    va_end(cp);
    return (s1 == s2) ? s1 : -1;
}

int main(void) {
    return sum(3, 10, 11, 21) - 42;
}
