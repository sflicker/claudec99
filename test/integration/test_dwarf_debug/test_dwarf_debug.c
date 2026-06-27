static int add(int a, int b) {
    int result;
    result = a + b;
    return result;
}

static int mul(int a, int b) {
    int i;
    int acc;
    acc = 0;
    for (i = 0; i < b; i++) {
        acc = add(acc, a);
    }
    return acc;
}

int main(void) {
    int x;
    int y;
    x = mul(3, 4);
    y = add(x, 2);
    if (y != 14) return 1;
    return 42;
}
