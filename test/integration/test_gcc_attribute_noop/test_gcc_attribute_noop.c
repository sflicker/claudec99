/* Stage 159: __attribute__ is a no-op macro so GCC-annotated declarations parse. */
static int add(int a, int b) __attribute__((noinline));

static int add(int a, int b) {
    return a + b;
}

int main(void) {
    int r = add(20, 22);
    if (r != 42) return 1;
    return r;
}
