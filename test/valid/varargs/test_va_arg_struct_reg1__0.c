#include <stdarg.h>

typedef struct { int x; int y; } Point;   /* 8 bytes — 1 GP eightbyte */

static Point first(int n, ...) {
    va_list ap;
    va_start(ap, n);
    Point p = va_arg(ap, Point);
    va_end(ap);
    return p;
}

int main(void) {
    Point p;
    p.x = 3; p.y = 7;
    Point r = first(1, p);
    return (r.x == 3 && r.y == 7) ? 0 : 1;
}
