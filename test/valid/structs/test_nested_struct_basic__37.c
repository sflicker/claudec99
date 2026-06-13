struct Point {
    int x;
    int y;
};

struct Rect {
    struct Point origin;
    int width;
    int height;
};

int main() {
    struct Rect r;

    r.origin.x = 10;
    r.origin.y = 20;
    r.width = 3;
    r.height = 4;

    return r.origin.x + r.origin.y + r.width + r.height;
}
