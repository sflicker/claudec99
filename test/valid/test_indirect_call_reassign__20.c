int inc(int x) {
    return x + 1;
}

int dec(int x) {
    return x - 1;
}

int main() {
    int (*fp)(int) = inc;
    int a = fp(10);    /* 11 */

    fp = dec;
    int b = dec(10);   /* 9 */

    return a + b;      /* expect 20 */
}
