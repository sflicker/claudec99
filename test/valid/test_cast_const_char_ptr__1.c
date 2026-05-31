int main(void) {
    char *p;
    const char *q;

    p = "hello";
    q = (const char *)p;

    return q[0] == 'h';       /* expect 1 */
}
