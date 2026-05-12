struct Point {
    int x;
    int y;
};

int get_x(struct Point *p) {
    return (*p).x;
}

int main() {
    struct Point p;
    p.x = 21;
    p.y = 21;
    return get_x(&p);  /* expect 21 */
}
