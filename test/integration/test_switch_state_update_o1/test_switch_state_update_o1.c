#include <stdio.h>
/* Test: switch loop where each case updates a loop-carried state variable.
   Verifies that -O1 preserves state transitions across switch cases. */
int main(void) {
    int state = 0;
    int total = 0;
    int i;
    for (i = 0; i < 4; i = i + 1) {
        switch (state) {
        case 0:
            total = total + 1;
            state = 1;
            break;
        case 1:
            total = total + 10;
            state = 2;
            break;
        case 2:
            total = total + 100;
            state = 0;
            break;
        }
    }
    /* state transitions: 0->1->2->0->1; totals: 1+10+100+1=112 */
    printf("%d\n", total);
    return 0;
}
