/* Calling a variadic function with no arguments is invalid when it has a fixed param. */
int f(int x, ...) {
    return x;
}

int main(void) {
    f();
    return 0;
}
