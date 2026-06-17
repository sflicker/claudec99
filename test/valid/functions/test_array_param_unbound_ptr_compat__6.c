/* CC99-008: int a[] and int *a are compatible parameter declarations. */
int sum3(int a[]);
int sum3(int *a) {
    return a[0] + a[1] + a[2];
}

int main(void) {
    int v[3] = { 1, 2, 3 };
    return sum3(v);
}
