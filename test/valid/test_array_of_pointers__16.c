//
// Created by scott on 4/28/26.
//
int main() {
    int x = 7;
    int y = 9;
    int *arr[2];
    arr[0] = &x;
    arr[1] = &y;
    return *arr[0] + *arr[1];  // expect 16
}