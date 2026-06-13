struct S {
    int x;
};

int main(void) {
    struct S s;
    int *p;

    s.x = 42;
    p = &s.x;

    return *p;   /* expect 42 */
}
