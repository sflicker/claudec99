struct S {
    int values[3];
};

int main(void) {
    struct S s;
    return s.values[0].missing;
}
