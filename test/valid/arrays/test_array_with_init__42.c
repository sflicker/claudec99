/* Array with an initializer */

int main() {
    int a[3] = {10, 30, 2};
    int sum = 0;
    for (int i=0;i<3;i++) {
        sum += a[i];
    }
    return sum;    // 42
}

