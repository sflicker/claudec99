struct S {
    int table[4][8];
};

int main() {
    struct S s;
    return sizeof(s.table[0]);
}
