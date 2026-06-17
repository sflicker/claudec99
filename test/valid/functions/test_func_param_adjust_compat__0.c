/* CC99-008: int f(int cb(void)) and int f(int (*cb)(void)) are compatible.
 * C99 6.7.5.3p8: function parameter is adjusted to pointer-to-function. */
int apply(int cb(void));
int apply(int (*cb)(void));

int zero(void) { return 0; }

int apply(int (*cb)(void)) {
    return cb();
}

int main(void) {
    return apply(zero);
}
