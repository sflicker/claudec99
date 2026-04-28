int main() {
    int a[3];
    int *p = a;

    p[0] = 4;
    p[1] = 5;
    p[2] = 6;

    return p[0] + p[1] + p[2];
}
