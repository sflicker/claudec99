int main(void) {
    int x;
    int *p = &x;
    int *q = &x;
    return (p <= q) ? 1 : 0;   /* 1 — same address */
}
