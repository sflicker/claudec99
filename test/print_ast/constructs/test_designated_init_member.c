struct Point { int x; int y; };
int main() {
    struct Point p = { .y = 10, .x = 3 };
    return p.x;
}
