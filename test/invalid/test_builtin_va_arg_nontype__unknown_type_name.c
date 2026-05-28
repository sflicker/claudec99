int f(int first, ...) {
    int ap;
    int x;
    int y;

    __builtin_va_start(ap, first);
    x = __builtin_va_arg(ap, y);
    __builtin_va_end(ap);

    return x;
}
