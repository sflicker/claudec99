struct S {
    int x;
    int y;
};

int main(void) {
    const struct S s = {1, 2};
    s.x = 3;    /* invalid: base struct is const */
    return s.x;
}
