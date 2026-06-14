//
// Created by scott on 6/29/25.
//
int main() {
    int a[5][4];
    int i;
    int j;
    int sum;
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 4; j++) {
            a[i][j] = i;
        }
    }
    sum = 0;
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 4; j++) {
            sum += a[i][j];
        }
    }
    return sum;   // 40
}
