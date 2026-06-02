struct S {
    int table[4][8];
};

int main() {
    struct S s;
    s.table[0][0] = 42;
    return s.table[0][0];
}
