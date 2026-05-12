struct Point {
    int x;
    int y;
};

int sum_point(struct Point *p) {
    return p->x + p->y;
}

int main() {
    struct Point p;
    p.x = 11;
    p.y = 22;
    return sum_point(&p);  /* expect 33 */
}
