typedef int A[4];

int main() {
    A a, b;

    a[0] = 3;
    b[0] = 5;

    return a[0] + b[0]; /* expect 8 */
}
