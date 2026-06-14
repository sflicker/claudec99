int main(void) {
    int arr[5];
    int i;
    for (i = 0; i < 5; i++) arr[i] = i + 1;
    int *first = arr;
    int *last  = arr + 4;   /* points at the last element, not one-past */
    int count = 0;
    int *p = first;
    while (p <= last) {
        count++;
        p++;
    }
    return count;   /* 5 */
}
