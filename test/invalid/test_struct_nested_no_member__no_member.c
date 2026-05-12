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
    return r.origin.z;
}
