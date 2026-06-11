int cell(int r, int c) {
    static int m[2][3] = {{1, 2, 3}, {4, 5, 6}};
    return m[r][c];
}
int main() {
    return (cell(0, 0) == 1 && cell(0, 2) == 3 &&
            cell(1, 0) == 4 && cell(1, 2) == 6) ? 0 : 1;
}
