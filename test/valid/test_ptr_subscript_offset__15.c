int main() {
    int a[3];
    int *p = a + 1;

    p[0] = 7;
    p[1] = 8;

    return a[1] + a[2];
}
