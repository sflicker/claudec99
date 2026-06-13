struct Pair { int x; int y; };
struct Pair g = { .y = 5, .x = 3 };
int main() { return g.x + g.y; }
