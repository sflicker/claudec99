#include <stdint.h>

int main(void) {
    uint8_t x;

    x = 250U;
    x += 10U;

    return x == 4U;
}
