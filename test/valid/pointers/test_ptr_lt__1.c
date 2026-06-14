int main(void) {
    int arr[2];
    int *lo = arr;
    int *hi = arr + 1;
    return (lo < hi) ? 1 : 0;   /* 1 */
}
