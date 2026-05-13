struct Point {
    int x;
    int y;
};

int main() {
    struct Point pt;
    struct Point *pp;
    void *vp;

    pt.x = 10;
    pt.y = 20;

    pp = &pt;
    vp = pp;
    pp = vp;

    return pp->x + pp->y;
}
