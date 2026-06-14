struct Vec2 { double x; double y; };
static struct Vec2 node;
static struct Vec2 *gp;   /* file-scope pointer, initialized in main */

int main(void) {
    gp = &node;
    gp->x = 4.0;
    gp->y = 5.0;
    gp->x += gp->y;      /* 4.0 + 5.0 = 9.0 */
    return (int)gp->x;
}
