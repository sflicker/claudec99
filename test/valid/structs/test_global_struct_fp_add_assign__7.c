struct Vec2 { double x; double y; };
struct Vec2 g;

int main(void) {
    g.x = 3.0;
    g.y = 4.0;
    g.x += g.y;          /* 3.0 + 4.0 = 7.0 */
    return (int)g.x;
}
