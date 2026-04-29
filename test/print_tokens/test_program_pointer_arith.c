int main() {
    int a[3];
    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    int *p = a + 1;
    return p[0] + p[1];
}
