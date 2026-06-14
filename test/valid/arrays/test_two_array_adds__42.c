int main() {
    int a[7];
    int b[7];

    // set initial values
    for (int i=0;i<=6;i++) {
        a[i] = i;
        b[i] = i;
    }

    // add arrays.
    int sum = 0;
    for (int i=0;i<=6;i++) {
        sum += a[i] + b[i];
    }

    return sum;
}
