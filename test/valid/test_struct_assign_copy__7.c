struct Point {
    int x;
    int y;
};

int main() {
    struct Point c = { 3, 4};
    struct Point d;

    d = c;

    return d.x + d.y;
}
