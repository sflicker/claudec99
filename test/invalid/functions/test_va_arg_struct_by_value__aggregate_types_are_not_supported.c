struct Point {
    int x;
    int y;
};

int f(int ignored, ...) {
    int ap;

    __builtin_va_start(ap, ignored);
    __builtin_va_arg(ap, struct Point);
    __builtin_va_end(ap);

    return 0;
}

int main(void) {
    return f(0);
}
