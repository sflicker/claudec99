int main() {
    long a[3];
    long *p = &a[2];
    *(p - 1) = 17;
    return a[1];
}
