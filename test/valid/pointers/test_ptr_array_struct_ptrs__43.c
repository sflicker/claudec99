struct Point {
    int x;
    int y;
};

int main() {
    struct Point a;
    struct Point b;
    struct Point *items[2];

    a.x = 1;
    a.y = 2;
    b.x = 10;
    b.y = 30;

    items[0] = &a;
    items[1] = &b;

    return items[0]->x + items[0]->y + items[1]->x + items[1]->y; // expect 43: 1+2+10+30
}
