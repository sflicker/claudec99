struct S {
    volatile int flag;
};

int main(void) {
    struct S s;
    s.flag = 42;
    return s.flag;    /* expect 42 */
}
