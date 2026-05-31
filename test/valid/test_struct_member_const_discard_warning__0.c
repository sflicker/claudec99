struct S {
    const char *name;
};

int main(void) {
    struct S s;
    char *p;
    s.name = "abc";
    p = s.name;     /* warning: discards const qualifier */
    return 0;
}
