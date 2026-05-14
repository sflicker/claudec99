typedef int *IntPtr;

int main() {
    int a;
    int b;
    IntPtr items[2];

    a = 5;
    b = 6;

    items[0] = &a;
    items[1] = &b;

    return *items[0] + *items[1]; // expect 11
}
