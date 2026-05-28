int f(int first, ...) {
    int ap;
    int x;

    __builtin_va_start(ap, first);
    x = __builtin_va_arg(ap, int);
    __builtin_va_end(ap);

    return x;
}
