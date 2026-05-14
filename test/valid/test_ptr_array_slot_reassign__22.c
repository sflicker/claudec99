int main() {
    int a;
    int b;
    int *items[1];

    a = 11;
    b = 22;

    items[0] = &a;
    items[0] = &b;

    return *items[0]; // expect 22
}
