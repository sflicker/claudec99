struct S {
    const char *p;
};

int main() {
    struct S s;
    s.p = "hello";
    return s.p[1] == 'e';
}
