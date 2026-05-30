int main(void) {
    int xs[3];
    int total;
    int n;

    xs[0] = 10;
    xs[1] = 20;
    xs[2] = 12;
    n = 3;

    total = 0;

    for (int *p = xs; n > 0; n = n - 1) {
        total = total + *p;
        p = p + 1;
    }

    return total;
}
