int main() {
    int x = 42;
    int *p = &x;
    return p ? *p : 0;
}
