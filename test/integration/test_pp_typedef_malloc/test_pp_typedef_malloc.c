#include <stdio.h>
#include <stdlib.h>

typedef struct Point { int x; int y; } Point;

int main(void) {
    Point *p = malloc(sizeof(Point));
    p->x = 10;
    p->y = 20;
    printf("%d\n", p->x + p->y);
    free(p);
    return 0;
}
