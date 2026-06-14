//
// Created by scott on 6/29/25.
//

int main() {
    int x = 10;
    int y = 20;
    int *p = &x;
    *p = *p + 5;      // x = 15
    p = &y;
    *p = *p + 5;      // y = 25
    return x + y;     // 40
}

