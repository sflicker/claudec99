#include <stdarg.h>

typedef struct { long a; long b; long c; } Big;   /* 24 bytes — MEMORY class */

static Big extract(int n, ...) {
    va_list ap;
    va_start(ap, n);
    Big s = va_arg(ap, Big);
    va_end(ap);
    return s;
}

int main(void) {
    Big b;
    b.a = 10; b.b = 20; b.c = 30;
    Big r = extract(1, b);
    if (r.a == 10 && r.b == 20 && r.c == 30) return 0;
    return 1;
}
