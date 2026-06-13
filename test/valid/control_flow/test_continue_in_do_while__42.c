int main() {
    int sum = 0;
    int i = 0;
    do {
        i = i + 1;
        if (i == 3) {
            continue;
        }
        sum = sum + i;
    } while (i < 9);
    return sum;
}
