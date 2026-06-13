int f(const char *fmt, ...) {
    int ap;
    __builtin_va_start(ap, fmt);
    __builtin_va_end(ap);
    return 42;
}

int main(void) {
    return f("%d", 1);
}
