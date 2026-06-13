int main(void) {
    int total;

    total = 0;

    for (int i = 0, j = 10; i < 4; i++) {
        total = total + i + j;
    }

    return total;
}
