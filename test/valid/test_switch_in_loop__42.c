int main() {
    int sum = 0;
    int i = 0;
    while (i < 6) {
        switch (i) {
            default:
                sum = sum + i;
        }
        i = i + 1;
    }
    int bonus = 0;
    switch (1) {
        default:
            bonus = 27;
    }
    return sum + bonus;
}
