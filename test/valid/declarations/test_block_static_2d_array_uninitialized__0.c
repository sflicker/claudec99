int bump(int r, int c) {
    static int grid[3][4];
    grid[r][c]++;
    return grid[r][c];
}
int main() {
    bump(0, 0); bump(0, 0);
    return (bump(0, 0) == 3 && bump(1, 2) == 1) ? 0 : 1;
}
