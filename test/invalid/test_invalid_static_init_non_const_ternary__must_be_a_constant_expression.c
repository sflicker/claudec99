int g(int n) {
    static int x = n ? 1 : 0;
    return x;
}
int main(void) { return g(1); }
