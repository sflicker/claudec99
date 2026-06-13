int f(int x) { return x; }

int main() {
    long (*fp)(int);
    fp = f;    /* INVALID: return type mismatch */
    return 0;
}
