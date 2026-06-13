int main() {
    union {
        int a;
        char b;
    } u, v;
    u.a = 42;
    v = u;
    return v.a;
}
