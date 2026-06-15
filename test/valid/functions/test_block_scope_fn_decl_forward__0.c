int main(void) {
    int foo(int x);
    return foo(42) - 42;
}
int foo(int x) { return x; }
