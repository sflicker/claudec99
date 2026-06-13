struct Trio { int a; int b; int c; };
int main() {
    struct Trio t = { .b = 7 };
    return t.a + t.b + t.c;
}
