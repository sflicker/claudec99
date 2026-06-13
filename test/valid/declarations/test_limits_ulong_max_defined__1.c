#include <limits.h>

int main() {
#ifdef ULONG_MAX
    return 1;
#else
    return 0;
#endif
}
