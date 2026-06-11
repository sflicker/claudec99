void record(int r, int c) {
    static int hits[2][2];
    hits[r][c]++;
}
int total(int r, int c) {
    static int hits[2][2];
    return hits[r][c];
}
int main() { return 0; }
