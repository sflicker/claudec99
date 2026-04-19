int main() {
    int total = 0;
    int i = 0;
    do {
        i = i + 1;
        int j = 0;
        do {
            j = j + 1;
            total = total + 1;
        } while (j < 7);
    } while (i < 6);
    return total;
}
