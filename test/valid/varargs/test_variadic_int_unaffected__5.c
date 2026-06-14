#include <stdarg.h>
static int sum(int n, ...) {
    va_list ap; va_start(ap, n); int r = 0;
    while (n--) r += va_arg(ap, int);
    va_end(ap); return r;
}
int main(void) { return sum(3, 1, 2, 3) - 1; }
