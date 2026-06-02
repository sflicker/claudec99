struct S {
    int table[4][8];
};

int main(void) {
    struct S s;

    return s.table[0][0][0];
}
