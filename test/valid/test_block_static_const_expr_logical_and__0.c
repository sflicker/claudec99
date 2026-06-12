int main(void) {
    static int x = 1 && 1;
    static int y = 1 && 0;
    return x - 1 + y;
}
