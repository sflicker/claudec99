//
// Created by scott on 6/29/25.
//

// other variables are global to isolate the array in local
int sum;
int i;
int j;

int main() {
    int a[5][4];
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 4; j++) {
            a[i][j] = 1;
        }
    }
    sum = 0;
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 4; j++) {
            sum += a[i][j];
        }
    }
    return sum;   // 20
}
