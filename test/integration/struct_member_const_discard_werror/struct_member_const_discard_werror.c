struct S {
    const char *name;
};

int main(void) {
    struct S s;
    char *p;
    s.name = "abc";
    p = s.name;     /* discards const qualifier */
    return p[0];
}
