/* Array with an initializer */

int main() {
    int a[3];
    int sum = 0;
    int i = 0;

    a[0] = 10;
    a[1] = 30;
    a[2] = 2;

    sum += a[i];
    i++;

    sum += a[i];
    i++;

    sum += a[i];

    return sum;    // 42
}

