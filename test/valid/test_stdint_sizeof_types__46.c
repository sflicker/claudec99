#include <stdint.h>

int main(void) {
    int8_t a;
    uint8_t b;
    int16_t c;
    uint16_t d;
    int32_t e;
    uint32_t f;
    int64_t g;
    uint64_t h;
    intptr_t p;
    uintptr_t q;

    a = -1;
    b = 255;
    c = -2;
    d = 65535;
    e = -3;
    f = 42;
    g = -4;
    h = 99;
    p = -5;
    q = 100;

    return sizeof(a)    /* 1      */
        +  sizeof(b)    /* 1+1=2  */
        +  sizeof(c)    /* 2+2=4  */
        +  sizeof(d)    /* 4+2=6  */
        +  sizeof(e)    /* 6+4=10 */
        +  sizeof(f)    /* 10+4=14 */
        +  sizeof(g)    /* 14+8=22 */
        +  sizeof(h)    /* 22+8=30 */
        +  sizeof(p)    /* 30+8=38 */
        +  sizeof(q);   /* 38+8=46 */
}
