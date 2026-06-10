struct Point { int x; int y; };
int sum_point(struct Point p) { return p.x + p.y; }
int main() {
    return sum_point((struct Point){ .x = 3, .y = 4 });
}
