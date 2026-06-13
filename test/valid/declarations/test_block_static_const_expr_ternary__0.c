int main(void) {
    static int x = 1 ? 42 : 0;
    static int y = 0 ? 0 : 7;
    return x - 42 + y - 7;
}
