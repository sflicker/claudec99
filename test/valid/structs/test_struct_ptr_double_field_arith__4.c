struct Vec2 { double x; double y; };
int main(void) {
    struct Vec2 v;
    struct Vec2 *p = &v;
    p->x = 1.5;
    p->y = 2.5;
    double z = p->x + p->y;
    return (int)z;   /* 4 */
}
