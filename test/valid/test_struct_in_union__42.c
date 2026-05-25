struct Pair {
    int a;
    int b;
};

union U {
    int i;
    struct Pair p;
};

int main() {
    union U u;
    u.p.a = 10;
    u.p.b = 32;
    return u.p.a + u.p.b;
}
