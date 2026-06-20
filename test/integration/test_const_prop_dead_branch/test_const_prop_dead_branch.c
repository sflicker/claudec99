#include <stdio.h>
int main(void) {
    const int FLAG = 0;
    int result = 0;
    if (FLAG) { result = 99; } else { result = 10; }
    const int LIMIT = 5;
    int n = 0;
    while (LIMIT - 5) { n = 99; }
    printf("%d %d\n", result, n);
    return 0;
}
