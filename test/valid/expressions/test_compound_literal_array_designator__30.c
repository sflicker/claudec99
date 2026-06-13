int main() {
    int *a = (int[5]){ [2] = 10, [4] = 20 };
    return a[0] + a[2] + a[4];
}
