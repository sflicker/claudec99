struct S {
    int x;
};

int main(void) {
    struct S s;
    struct S *sp;
    int *p;

    sp = &s;
    sp->x = 42;
    p = &sp->x;

    return *p;   /* expect 42 */
}
