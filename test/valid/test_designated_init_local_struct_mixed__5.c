struct Four { int a; int b; int c; int d; };
int main() {
    struct Four f = { .b = 2, 3 };
    return f.a + f.b + f.c + f.d;
}
