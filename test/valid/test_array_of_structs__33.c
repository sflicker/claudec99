struct Point {
    int x;
    int y;
};

int main() {
    struct Point points[2];
    points[0].x = 1;
    points[0].y = 2;
    points[1].x = 10;
    points[1].y = 20;
    return points[0].x + points[0].y + points[1].x + points[1].y;
}
