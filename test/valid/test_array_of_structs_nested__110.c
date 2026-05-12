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
    struct Rect rects[2];

    rects[0].origin.x = 1;
    rects[0].origin.y = 2;
    rects[0].width = 3;
    rects[0].height = 4;

    rects[1].origin.x = 10;
    rects[1].origin.y = 20;
    rects[1].width = 30;
    rects[1].height = 40;

    return rects[0].origin.x
         + rects[0].origin.y
         + rects[0].width
         + rects[0].height
         + rects[1].origin.x
         + rects[1].origin.y
         + rects[1].width
         + rects[1].height;
}
