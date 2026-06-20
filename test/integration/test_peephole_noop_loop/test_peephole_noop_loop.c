#include <stdio.h>
int main(void) {
    int i;
    int s = 0;
    for (i = 1; i <= 5; i++) s += i;
    printf("%d\n", s);
    return 0;
}
