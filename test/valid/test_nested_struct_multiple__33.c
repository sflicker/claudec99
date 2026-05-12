struct Point {
    int x;
    int y;
};

struct Line {
    struct Point start;
    struct Point end;
};

int main() {
    struct Line line;

    line.start.x = 1;
    line.start.y = 2;
    line.end.x = 10;
    line.end.y = 20;

    return line.start.x + line.start.y + line.end.x + line.end.y;
}
