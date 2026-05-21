#define JOIN(a, b) a ## b

int main(void) {
    int foobar;
    foobar = 42;
    return JOIN(foo, bar);
}
