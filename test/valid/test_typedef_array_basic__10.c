typedef int A[4];

int main() {
    A a;
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    a[3] = 4;
    return a[0] + a[1] + a[2] + a[3]; /* expect 10 */
}
