#include <stdarg.h>

/* Tests C99 §6.5.2.2p7: default argument promotions in variadic calls.
 * Narrow integers and float are promoted before being passed as extras. */
static int check(int count, ...) {
    va_list ap;
    int score = 0;
    va_start(ap, count);

    if (va_arg(ap, int) == 65)    score += 1;  /* char -> int */
    if (va_arg(ap, int) == -1)    score += 2;  /* signed char -> int */
    if (va_arg(ap, int) == 200)   score += 4;  /* unsigned char -> int */
    if (va_arg(ap, int) == -1234) score += 8;  /* short -> int */
    if (va_arg(ap, int) == 60000) score += 16; /* unsigned short -> int */
    if ((int)(va_arg(ap, double) * 10.0) == 15) score += 32; /* float -> double */
    if ((int)(va_arg(ap, double) * 10.0) == 25) score += 64; /* double stays double */

    va_end(ap);
    return score;
}

int main(void) {
    char           a = 65;
    signed char    b = -1;
    unsigned char  c = 200;
    short          d = -1234;
    unsigned short e = 60000;
    float          f = 1.5f;
    double         g = 2.5;

    return check(7, a, b, c, d, e, f, g);
}
