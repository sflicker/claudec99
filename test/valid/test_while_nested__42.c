int main() {
    int a = 0;
    int i = 0;
    int j = 0;
    while (i < 6) {
        j = 0;
        while (j < 7) {
            a = a + 1;
            j = j + 1;
        }
        i = i + 1;
    }
    return a;
}
