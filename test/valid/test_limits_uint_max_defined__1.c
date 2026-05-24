#include <limits.h>

int main() {
#ifdef UINT_MAX
    return 1;
#else
    return 0;
#endif
}
