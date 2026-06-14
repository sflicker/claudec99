/* Array with an initializer */

int main() {
    int a[3];
    a[0] = 10;
    a[1] = 30;
    a[2] = 2;
    int sum = 0;
    int i = 0;
    sum = sum + a[i];
    i++;
    sum = sum + a[i];
    i++;
    sum = sum + a[i];
    return sum;    // 42
}

