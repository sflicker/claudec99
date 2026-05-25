int main() {
    struct {
        int x;
        int y;
    } p, q;

    p.x = 10;
    p.y = 32;
    q = p;
    return q.x + q.y;
}
