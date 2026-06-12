int main(void) {
    static int x = (sizeof(int) == 4);
    return x - 1;
}
