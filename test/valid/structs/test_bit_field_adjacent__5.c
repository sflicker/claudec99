/* Adjacent bit-fields sharing a storage unit. */
struct Packed {
    unsigned int a : 4;
    unsigned int b : 4;
    unsigned int c : 8;
    unsigned int d : 16;
};

int main(void) {
    struct Packed p;
    p.a = 3;
    p.b = 7;
    p.c = 250;
    p.d = 0;
    /* All four fields share one 32-bit storage unit. */
    return (int)(p.a + p.b - 5); /* 3 + 7 - 5 = 5 */
}
