struct Point {
    int x;
    int y;
};

int main() {
    struct Point p;
    struct Point *q;
    q = &p;
    q->x = 5;
    q->y = 6;
    return q->x + q->y;
}
