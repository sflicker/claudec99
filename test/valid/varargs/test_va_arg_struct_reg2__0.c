#include <stdarg.h>

typedef struct { long a; long b; } Pair;  /* 16 bytes — 2 GP eightbytes */

static Pair grab(int n, ...) {
    va_list ap;
    va_start(ap, n);
    Pair s = va_arg(ap, Pair);
    va_end(ap);
    return s;
}

int main(void) {
    Pair p;
    p.a = 100; p.b = 200;
    Pair r = grab(1, p);
    return (r.a == 100 && r.b == 200) ? 0 : 1;
}
