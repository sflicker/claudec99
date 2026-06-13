/* Stage 79: general-lvalue compound assignment through a pointer dereference. */
int main(void) {
    int x;
    int *p;

    x = 21;
    p = &x;
    *p *= 2;

    return x;     /* expect 42 */
}
