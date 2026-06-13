#include <limits.h>
#include <stdbool.h>

int main() {
    bool a = UINT_MAX < ULONG_MAX;
    return a;
}
