struct Point { int x; int y; };

int sum_coords(void) {
    static struct Point p = {3, 4};
    return p.x + p.y;
}

int main() {
    return sum_coords() - 7;
}
