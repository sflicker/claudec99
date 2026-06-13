int main() {
    int i = 0;
    while (i < 100) {
        switch (i) {
            default:
                i = i + 1;
                break;
                i = 1000;
        }
        if (i == 42) {
            break;
        }
    }
    return i;
}
