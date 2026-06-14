//
// Created by scott on 6/24/25.
//
int main() {
    int sum = 0;
    for (int i=0;i<10;i++) {
        if (i == 5) continue;
        if (i == 8) break;
        sum += i;
    }
    return sum;   // 0+1+2+3+4+6+7 = 23
}