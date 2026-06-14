struct Point { int x; int y; };
struct Point pts[3];
int main(void) {
    pts[0].x = 1; pts[0].y = 2;
    pts[1].x = 3; pts[1].y = 4;
    pts[2].x = 5; pts[2].y = 6;
    return pts[0].x + pts[1].y + pts[2].x;  /* 1 + 4 + 5 = 10 */
}
