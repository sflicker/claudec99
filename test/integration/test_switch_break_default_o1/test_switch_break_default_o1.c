#include <stdio.h>
/* Test: switch loop with a default case; each arm ends with break.
   Verifies that -O1 does not remove the default arm when case arms have breaks. */
static int classify(int x) {
    switch (x) {
    case 0:
        return 10;
        break;
    case 1:
        return 20;
        break;
    case 2:
        return 30;
        break;
    default:
        return 99;
        break;
    }
    return 0;
}

int main(void) {
    int a = classify(0);
    int b = classify(1);
    int c = classify(2);
    int d = classify(3);
    /* 10 + 20 + 30 + 99 = 159 */
    printf("%d\n", (a + b + c + d) & 255);
    return 0;
}
