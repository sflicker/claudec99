int main() {
    int x;
    int * const p = &x;
    p = &x;
    return 0;
}
