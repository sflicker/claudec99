int main() {
    int x = 7;
    int *p = &x;
    int *q = &x;
    return p == q;
}
