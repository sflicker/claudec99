int identity(int x) { return x; }

int main(void) {
    int v = identity(5);
    if (v != 5) return 1;
    return 42;
}
