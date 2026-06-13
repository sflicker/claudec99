int main() {
    int a;
    int b;
    int *items[2];

    a = 10;
    b = 20;

    items[0] = &a;
    items[1] = &b;

    return *items[0] + *items[1]; // expect 30
}
