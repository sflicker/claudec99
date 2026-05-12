struct Point {
    int x;
    int y;
};

int move(struct Point *p, int dx, int dy) {
    p->x = dx;
    p->y = dy;
    return 0;
}

int main() {
    struct Point p;
    p.x = 10;
    p.y = 20;
    move(&p, 3, 4);
    return p.x + p.y;  /* expect 7 */
}
