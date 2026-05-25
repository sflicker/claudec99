typedef union {
    int a;
    char b;
} U;

int main() {
    U u;
    u.a = 42;
    return u.a;
}
