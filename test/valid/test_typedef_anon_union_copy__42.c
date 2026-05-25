typedef union {
    int a;
    char b;
} U;

int main() {
    U u;
    U v;
    u.a = 42;
    v = u;
    return v.a;
}
