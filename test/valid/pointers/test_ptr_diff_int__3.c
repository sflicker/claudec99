int main() {
    int a[5];
    int *p;
    int *q;
    p = a;
    q = p + 3;
    return q - p;
}
