struct Point {
    int x;
    int y;
};

typedef struct Point Point;

int main() {
    Point p;
    p.x = 10;
    p.y = 20;
    return p.x + p.y;
}
