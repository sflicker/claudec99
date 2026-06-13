struct S {
    int table[4][8];
};

int main() {
    struct S s;
    struct S *p;

    p = &s;
    p->table[2][3] = 42;

    return p->table[2][3];
}
