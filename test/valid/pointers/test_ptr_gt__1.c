int main(void) {
    int arr[2];
    int *lo = arr;
    int *hi = arr + 1;
    return (hi > lo) ? 1 : 0;   /* 1 */
}
