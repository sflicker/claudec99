typedef int A[4];

int main() {
    A a;
    A *p = &a;

    (*p)[0] = 7;
    return a[0]; /* expect 7 */
}
