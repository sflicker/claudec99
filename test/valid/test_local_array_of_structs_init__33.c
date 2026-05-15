struct Point { int x; int y; };

int main() {
    struct Point points[2] = { {1, 2}, {10, 20} };
    return points[0].x + points[0].y + points[1].x + points[1].y;
}
