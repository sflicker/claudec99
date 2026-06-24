int pass_through(int a) { int x = a; return x; }

int main(void) {
    int v = pass_through(42);
    if (v != 42) return 1;
    return 42;
}
