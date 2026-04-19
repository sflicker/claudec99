int main() {
    int sum = 0;
    int i;
    int j;
    for (i = 0; i < 6; i += 1) {
        for (j = 0; j < 8; j += 1) {
            if (j == 0) {
                continue;
            }
            sum += 1;
        }
    }
    return sum;
}
