int f(int n) {
    static int x = n + 1;   /* INVALID: variable reference in static initializer */
    return x;
}
int main(void) { return f(0); }
