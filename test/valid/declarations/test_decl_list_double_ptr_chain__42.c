int main() {
    int a, *p, **q;
    a = 42;
    p = &a;
    q = &p;
    return **q;
}
