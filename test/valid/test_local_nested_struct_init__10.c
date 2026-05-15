struct Point { int x; int y; };
struct Rect { struct Point origin; int width; int height; };

int main() {
    struct Rect r = { {1, 2}, 3, 4 };
    return r.origin.x + r.origin.y + r.width + r.height;
}
