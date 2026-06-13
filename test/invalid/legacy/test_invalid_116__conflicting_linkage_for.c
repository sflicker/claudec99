static int f();   // INVALID: static then non-static

int f() {
    return 1;
}

int main() {
    return f();
}
