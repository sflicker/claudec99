struct Point {
    int x;
    int y;
};

int move(struct Point *p, int dx, int dy) {
    p->z = dx;
    return 0;
}

int main() {
    struct Point p;
    p.x = 1;
    p.y = 1;
    move(&p, 1, 1);
    return 0;
}
