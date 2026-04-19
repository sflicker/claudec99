int main() {
    int i;
    for (i = 0; i < 100; i += 1) {
        if (i == 42) {
            break;
        }
    }
    return i;
}
