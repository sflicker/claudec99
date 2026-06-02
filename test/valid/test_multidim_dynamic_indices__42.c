struct S {
    int table[4][8];
};

int main(void) {
    struct S s;
    int i = 3;
    int j = 5;

    s.table[i][j] = 42;
    return s.table[3][5];
}
