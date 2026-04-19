int main() {
    int sum = 0;
    int i = 0;
    while (i < 9) {
        i = i + 1;
        if (i == 3) {
            continue;
        }
        sum = sum + i;
    }
    return sum;
}
