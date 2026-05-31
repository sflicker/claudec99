struct S {
    const char *p;
};

int main(void) {
    struct S s;
    s.p = "hello";
    s.p[0] = 'H';
    return 0;
}
