struct Point { int x; int y; };
int main() {
    struct Point p = (struct Point){ .x = 10, .y = 20 };
    return p.x + p.y;
}
