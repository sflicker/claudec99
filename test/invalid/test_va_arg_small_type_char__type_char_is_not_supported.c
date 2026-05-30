int f(int ignored, ...) {
    int ap;
    char c;

    __builtin_va_start(ap, ignored);
    c = __builtin_va_arg(ap, char);
    __builtin_va_end(ap);

    return c;
}

int main(void) {
    return f(0, 65);
}
