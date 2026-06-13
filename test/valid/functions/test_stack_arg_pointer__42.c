int f(int a, int b, int c, int d, int e, int g, int *p) {
    return *p;
}

int main(void) {
    int x;
    x = 42;
    return f(0, 0, 0, 0, 0, 0, &x);
}
