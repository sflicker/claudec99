struct Vec2 { double x; double y; };
int main(void) {
    struct Vec2 v;
    v.x = 1.5;
    v.y = 2.5;
    double z = v.x + v.y;
    return (int)z;   /* 4 */
}
