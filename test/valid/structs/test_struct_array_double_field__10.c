struct Point { double x; double y; };
static struct Point pts[3];
int main(void) {
    pts[0].x = 1.0; pts[0].y = 2.0;
    pts[1].x = 3.0; pts[1].y = 4.0;
    pts[2].x = 5.0; pts[2].y = 6.0;
    double sum = pts[0].x + pts[1].y + pts[2].x;
    return (int)sum;   /* 1 + 4 + 5 = 10 */
}
