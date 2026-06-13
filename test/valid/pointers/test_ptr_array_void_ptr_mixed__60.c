struct Point {
    int x;
    int y;
};

int main() {
    int x;
    struct Point p;
    int *ip;
    struct Point *pp;
    void *items[2];

    x = 10;
    p.x = 20;
    p.y = 30;

    items[0] = &x;
    items[1] = &p;

    ip = items[0];
    pp = items[1];

    return *ip + pp->x + pp->y; // expect 60
}
