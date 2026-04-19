int main() {
    int sum = 0;
    int i;
    int j;
    for (i = 0; i < 6; i += 1) {
        for (j = 0; j < 100; j += 1) {
            if (j == 7) {
                break;
            }
            sum += 1;
        }
    }
    return sum;
}
