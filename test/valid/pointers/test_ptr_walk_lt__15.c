int main(void) {
    int arr[5];
    int i;
    for (i = 0; i < 5; i++) arr[i] = i + 1;   /* 1 2 3 4 5 */
    int *p = arr;
    int *end = arr + 5;
    int sum = 0;
    while (p < end) {
        sum += *p;
        p++;
    }
    return sum;   /* 15 */
}
