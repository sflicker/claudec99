int add(int a, int b) { return a + b; }

int main(void) {
    int x = add(10, 11);
    int y = add(x, 1);
    if (y != 22) return 1;
    return 42;
}
