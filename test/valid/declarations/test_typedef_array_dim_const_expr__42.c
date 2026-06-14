#define N 10
typedef int Row[N * 2];
int main(void) {
    Row r;
    r[0] = 42;
    return r[0];
}
