#define ROWS 3
#define COLS 4
int main(void) {
    int a[ROWS + 1][COLS + 2];
    a[0][0] = 42;
    return a[0][0];
}
