int main(void) {
    int total;

    total = 0;

    for (int i = 0; i < 5; i = i + 1) {
        if (i < 3) {
            continue;
        }

        total = total + i;
    }

    return total;
}
