int overwrite(int a, int b) {
    int x = a;
    x = b;
    return x;
}

int main(void) {
    int v = overwrite(99, 42);
    if (v != 42) return 1;
    return 42;
}
