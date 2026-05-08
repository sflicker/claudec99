int f(int x) { return x; }

int main() {
    int (*fp)(long);
    fp = f;    /* INVALID: parameter type mismatch */
    return 0;
}
