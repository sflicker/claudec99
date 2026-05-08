int main() {
    int x = 3;
    int *p = &x;
    return p(1);   /* INVALID: pointer-to-int is not callable */
}
