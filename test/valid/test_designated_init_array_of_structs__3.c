struct P { int x; int y; };
struct P pts[3] = { [1] = { .y = 2 }, [0] = { .x = 1 } };
int main() {
    return pts[0].x + pts[0].y + pts[1].x + pts[1].y + pts[2].x + pts[2].y;
}
