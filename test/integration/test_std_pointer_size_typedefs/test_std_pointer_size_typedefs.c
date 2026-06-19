#include <stddef.h>
#include <stdint.h>

int main(void) {
    int values[5] = { 1, 2, 3, 4, 5 };
    int *first = &values[0];
    int *last = &values[4];
    size_t object_size = sizeof values;
    size_t element_size = sizeof values[0];
    ptrdiff_t forward = last - first;
    ptrdiff_t backward = first - last;
    intptr_t address = (intptr_t)last;
    int *again = (int *)address;

    if (object_size / element_size != 5) {
        return 101;
    }
    if (sizeof(size_t) != sizeof(void *)) {
        return 102;
    }
    if (sizeof(ptrdiff_t) != sizeof(void *)) {
        return 103;
    }
    if (sizeof(intptr_t) != sizeof(void *)) {
        return 104;
    }
    if (!((size_t)0 - (size_t)1 > 0)) {
        return 105;
    }
    if (forward != 4) {
        return 106;
    }
    if (backward != -4) {
        return 107;
    }
    if (sizeof(forward) != sizeof(ptrdiff_t)) {
        return 108;
    }
    if (again != last) {
        return 109;
    }
    if (*again != 5) {
        return 110;
    }

    return 10;
}
