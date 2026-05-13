int main() {
    int x;
    int *p;
    void *v;

    x = 42;
    p = &x;
    v = p;
    p = v;

    return *p;
}
