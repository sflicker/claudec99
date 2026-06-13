struct Point {
    int x;
    int y;
};

int main() {
    struct Point points[2];
    struct Point *p;

    points[0].x = 1;
    points[0].y = 2;
    points[1].x = 10;
    points[1].y = 20;

    p = points;
    return (p + 1)->x + (p + 1)->y;
}
