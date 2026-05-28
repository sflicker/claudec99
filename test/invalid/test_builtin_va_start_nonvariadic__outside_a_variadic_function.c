int f(const char *fmt) {
    int ap;
    __builtin_va_start(ap, fmt);
    __builtin_va_end(ap);
    return 0;
}
