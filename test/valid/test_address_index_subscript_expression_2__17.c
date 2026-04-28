int main() {
    int a[3];
    int *p = a;
    int *q = &p[2];
    *q = 17;
    return a[2];

}