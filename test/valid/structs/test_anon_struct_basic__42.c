int main() {
    struct {
        int x;
        int y;
    } p;

    p.x = 10;
    p.y = 32;
    return p.x + p.y;
}
