struct Point { int x; int y; };
int sum_all(void) {
    static struct Point pts[3] = {{1, 2}, {3, 4}, {5, 6}};
    return pts[0].x + pts[0].y + pts[1].x + pts[1].y +
           pts[2].x + pts[2].y;
}
int main() {
    return sum_all() - 21;  /* 1+2+3+4+5+6 = 21; expect 0 */
}
