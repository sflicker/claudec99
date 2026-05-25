int a(void) {
    static int x;
    x = x + 1;
    return x;
}

int b(void) {
    static int x;
    x = x + 10;
    return x;
}

int main() {
    return a() + b() + a() + b();  // 1 + 10 + 2 + 20 = 33
}
