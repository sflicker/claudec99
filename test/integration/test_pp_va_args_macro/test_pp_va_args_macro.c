#include <stdio.h>

#define PRINT(...) printf(__VA_ARGS__)

int main(void) {
    PRINT("value = %d\n", 42);
    return 0;
}
