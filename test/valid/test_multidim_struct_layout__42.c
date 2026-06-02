struct S {
    char tag;
    int table[4][8];
};

int main() {
    struct S s;

    s.tag = 1;
    s.table[0][0] = 41;

    return s.tag + s.table[0][0];
}
