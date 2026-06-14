#include <stdlib.h>

static int arr_cmp(const void *a, const void *b) {
    /* Uses array subscript on the cast pointer — triggers emit_array_index_addr
     * which uses the push/pop rbx pattern. */
    const int *p = (const int *)a;
    const int *q = (const int *)b;
    int x = p[0];    /* subscript: uses rbx scratch */
    int y = q[0];
    return x - y;
}

int main(void) {
    int data[5] = {5, 2, 8, 1, 9};
    qsort(data, 5, sizeof(int), arr_cmp);
    return (data[0] == 1 && data[4] == 9) ? 0 : 1;
}
