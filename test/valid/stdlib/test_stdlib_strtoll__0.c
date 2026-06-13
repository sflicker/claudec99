#include <stdlib.h>
int main(void) {
    long long a = strtoll("9223372036854775807", 0, 10);
    unsigned long long b = strtoull("18446744073709551615", 0, 10);
    return (a == 9223372036854775807LL && b == 18446744073709551615ULL) ? 0 : 1;
}
