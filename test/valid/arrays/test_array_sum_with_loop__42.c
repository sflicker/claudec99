/* Array with an initializer */

int main() {
    int a[3];
    a[0] = 10;
    a[1] = 30;
    a[2] = 2;
    int sum = 0;
    for (int i=0;i<3;i++) {
        sum += a[i];
    }
    return sum;    // 42
}

