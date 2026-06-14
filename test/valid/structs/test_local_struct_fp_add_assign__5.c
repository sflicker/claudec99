struct Vec2 { double x; double y; };

int main(void) {
    struct Vec2 v;
    v.x = 2.0;
    v.y = 3.0;
    v.x += v.y;          /* 2.0 + 3.0 = 5.0 */
    return (int)v.x;
}
