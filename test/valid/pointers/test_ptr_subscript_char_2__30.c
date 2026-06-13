int main() {
    char a[2];
    char *p = a;

    p[0] = 10;
    p[1] = 20;

    return p[0] + p[1];
}
