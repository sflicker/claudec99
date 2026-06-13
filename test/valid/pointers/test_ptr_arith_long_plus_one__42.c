int main() {
    long a[2];
    long *p = a;
    *(p + 1) = 42;
    return a[1];
}
