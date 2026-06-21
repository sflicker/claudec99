/* Stage 159: qualifier after base type in function params (C99 §6.7).
 * 'void const *' has the qualifier trailing the type specifier. */
typedef unsigned long size_t;
extern void *memcpy(void *dest, void const *src, size_t n);

static int accepts_const_void_ptr(void const *p, size_t n) {
    (void)p;
    return (int)n;
}

int main(void) {
    char dst[4];
    char src[4] = {10, 20, 30, 40};
    memcpy(dst, src, 4);
    if (dst[0] != 10 || dst[3] != 40) return 1;
    int r = accepts_const_void_ptr(dst, 42);
    if (r != 42) return 2;
    return 42;
}
