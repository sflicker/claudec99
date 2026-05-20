#include "math.h"
#include "settings.h"

int main() {
#if FUNC == 1
    return add(A, B);
#else
    return 0;
#endif
}
