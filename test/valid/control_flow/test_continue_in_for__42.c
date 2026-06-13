int main() {
    int sum = 0;
    int i;
    for (i = 1; i <= 9; i += 1) {
        if (i == 3) {
            continue;
        }
        sum += i;
    }
    return sum;
}
