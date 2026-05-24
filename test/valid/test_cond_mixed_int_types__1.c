int main(void) {
    unsigned long x;
    int cond;

    cond = 1;
    x = cond ? 42U : 9L;

    return x == 42UL;
}
