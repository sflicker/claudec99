int f(void) {
    if (1) {
        static int x;
        x = x + 1;
        return x;
    }
    return 0;
}

int main() {
    return f() + f();   // expect 3
}
