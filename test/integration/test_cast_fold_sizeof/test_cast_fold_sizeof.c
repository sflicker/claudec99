#include <stdio.h>
int main(void) {
    /* sizeof(long)->8L; (int)8L->8; 8+1->9 (stage 143) */
    int a = (int)sizeof(long) + 1;
    /* sizeof(int)->4L; (int)4L->4; 4*2->8 (stage 143) */
    int b = (int)sizeof(int) * 2;
    /* sizeof(char)->1L; (long)1L->1L; 1L+7L->8L (stage 143) */
    long c = (long)sizeof(char) + 7L;
    printf("%d %d %ld\n", a, b, c);
    return 0;
}
