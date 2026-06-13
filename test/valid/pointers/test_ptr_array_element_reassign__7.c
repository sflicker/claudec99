int main() {
    int a;
    int b;
    int *items[2];

    a = 7;
    b = 9;

    items[0] = &a;
    items[1] = items[0];

    return *items[1]; // expect 7
}
