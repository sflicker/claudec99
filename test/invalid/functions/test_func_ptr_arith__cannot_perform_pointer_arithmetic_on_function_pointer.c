int foo(int x) { return x; }
int main() {
    int (*fp)(int) = foo;
    fp = fp + 1;
    return 0;
}
