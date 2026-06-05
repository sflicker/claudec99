/* Stage 91-01: a memory-class struct (> 16 bytes) passed and returned by value
 * via the SysV hidden-pointer (sret) convention. bump() mutates its private
 * copy; the caller's original is unchanged. Expected exit code: 42. */
#include <stdio.h>

typedef struct Big {
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;   /* 24 bytes -> memory class */
} Big;

static Big bump(Big x) {
    x.a = x.a + 1;
    x.f = 100;
    return x;
}

int main(void) {
    Big b;
    Big r;
    b.a = 10; b.b = 2; b.c = 3; b.d = 4; b.e = 5; b.f = 6;

    r = bump(b);

    if (b.a != 10) return 91;   /* caller's original untouched */
    if (b.f != 6)  return 92;
    if (r.a != 11) return 93;   /* returned copy was modified */
    if (r.f != 100) return 94;
    if (r.b != 2 || r.e != 5) return 95;
    return 42;
}
