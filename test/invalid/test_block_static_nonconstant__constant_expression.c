int f(void) {
    int y = 3;
    static int x = y;   /* INVALID: non-constant initializer */
    return x;
}
