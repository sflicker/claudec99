int main() {
    int a[4] = { [3] = 9, [0] = 1 };
    return a[0] + a[1] + a[2] + a[3];
}
