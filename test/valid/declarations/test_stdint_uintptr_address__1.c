#include <stdint.h>
#include <stddef.h>

int main(void) {
    int32_t x;
    uintptr_t p;

    x = 42;
    p = (uintptr_t)&x;

    return p != NULL;
}
