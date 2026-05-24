#include <limits.h>
int main(void) {
#if defined(CLAUDEC99_LIMITS_H) && defined(LLONG_MIN)
    return 0;
#else
    return 1;
#endif
}
