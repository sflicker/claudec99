struct S {
    int table[4][8];
};

int main(void) {
    struct S s;
    return sizeof(s.table[0]) == 32;
}
