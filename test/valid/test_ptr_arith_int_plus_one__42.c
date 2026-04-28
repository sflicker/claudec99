int main() {
    int a[3];
    int *p = a;
    *(p + 1) = 42;
    return a[1];
}
