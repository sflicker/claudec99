int main() {
    int a = 0;
    do {
        a = a + 1;
        if (a == 42) {
            break;
        }
    } while (a < 100);
    return a;
}
