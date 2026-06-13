struct Point { int x; int y; };
int dot(struct Point p, struct Point q) { return p.x * q.x + p.y * q.y; }
int main() {
    int result = dot(
        (struct Point){ .x = 2, .y = 3 },
        (struct Point){ .x = 4, .y = 5 }
    );
    return result;
}
