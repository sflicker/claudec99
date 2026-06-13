/* Stage 79: general-lvalue compound assignment through an arrow access. */
struct Buf {
    int cap;
};

int main(void) {
    struct Buf b;
    struct Buf *p;

    p = &b;
    p->cap = 21;
    p->cap *= 2;

    return p->cap;         /* expect 42 */
}
