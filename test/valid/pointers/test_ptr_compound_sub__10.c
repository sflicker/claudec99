int main() {
    int a[3];
    int *p;
    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    p = a + 1;
    p -= 1;
    return *p;
}
