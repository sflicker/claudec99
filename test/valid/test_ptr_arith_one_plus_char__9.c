int main() {
    char a[2];
    char *p = a;
    *(1 + p) = 9;
    return a[1];
}
