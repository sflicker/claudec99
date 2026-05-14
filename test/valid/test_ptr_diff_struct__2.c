struct Point {
    int x;
    int y;
};

int main() {
    struct Point pts[4];
    struct Point *p;
    struct Point *q;
    p = pts;
    q = p + 2;
    return q - p;
}
