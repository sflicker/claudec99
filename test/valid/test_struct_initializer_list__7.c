struct Point {
    int x;
    int y;
};

int main() {
    struct Point p = {3, 4};
    return p.x + p.y;
}
