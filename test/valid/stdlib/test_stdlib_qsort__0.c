#include <stdlib.h>
#include <string.h>
static int cmp(const void *a, const void *b) {
    const int *ia = (const int *)a;
    const int *ib = (const int *)b;
    return *ia - *ib;
}
int main(void) {
    int arr[5] = {5, 3, 1, 4, 2};
    int expected[5] = {1, 2, 3, 4, 5};
    qsort(arr, 5, sizeof(int), cmp);
    return memcmp(arr, expected, sizeof(arr)) ? 1 : 0;
}
