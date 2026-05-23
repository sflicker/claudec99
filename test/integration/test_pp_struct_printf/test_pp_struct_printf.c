#include <stdio.h>

#define POINT_X 3
#define POINT_Y 4

struct Point {
    int x;
    int y;
};

int main(void) {
    struct Point p;
    p.x = POINT_X;
    p.y = POINT_Y;
    printf("%d %d\n", p.x, p.y);
    return 0;
}
