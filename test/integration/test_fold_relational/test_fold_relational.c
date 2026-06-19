#include <stdio.h>
int main(void) {
    int lt = 3 < 5;
    int le = 5 <= 5;
    int gt = 7 > 3;
    int ge = 3 >= 4;
    int eq = 4 == 4;
    int ne = 4 != 5;
    printf("%d %d %d %d %d %d\n", lt, le, gt, ge, eq, ne);
    return 0;
}
