#include <limits.h>
int main(void) {
#if defined(CLAUDEC99_LIMITS_H) && defined(ULLONG_MAX)
    return 0;
#else
    return 1;
#endif
}
