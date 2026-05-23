#define CONCAT(a, b) a##b

int main(void) {
    int CONCAT(foo, bar) = 42;
    return foobar;
}
