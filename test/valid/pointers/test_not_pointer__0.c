int main(void) {
    int a = 2;
    int *p = &a;
    int *q = 0;
    if (!p) {
        return 1;
    }
    if (!q) {
        return 0;
    }
    return 2;
}
