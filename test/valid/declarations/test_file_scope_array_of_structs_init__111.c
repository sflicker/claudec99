struct Point { int x; int y; };

struct Point points[] = {
    {1, 2},
    {10, 20},
    {100, 200}
};

int main() {
    return points[0].x + points[1].x + points[2].x;
}
