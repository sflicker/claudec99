int main() {
    int x = 1;
    int y = 2;
    int *p = &x;
    int *q = &y;
    return p != q;
}
