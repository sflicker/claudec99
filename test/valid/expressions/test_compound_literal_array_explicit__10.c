int sum_arr(int *a, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) s += a[i];
    return s;
}
int main() {
    return sum_arr((int[4]){ 1, 2, 3, 4 }, 4);
}
