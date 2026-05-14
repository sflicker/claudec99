int main() {
    int a;
    int b;
    int *items[2];

    a = 1;
    b = 2;

    items[0] = &a;
    items[1] = &b;

    *items[0] = 10;
    *items[1] = 20;

    return a + b; // expect 30
}
