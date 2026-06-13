int main() {
    int a[5] = { [2] = 10, [4] = 20 };
    return a[0] + a[1] + a[2] + a[3] + a[4];
}
