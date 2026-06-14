struct Pair { int x; int y; };
int sum(void) {
    static struct Pair data[3];
    data[0].x = 1; data[1].x = 2; data[2].x = 4;
    return data[0].x + data[1].x + data[2].x;
}
int main(void) { return sum(); }
