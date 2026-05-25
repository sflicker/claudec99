int main() {
    union {
        int a;
        char b;
    } u;
    u.a = 42;
    return u.a;
}
