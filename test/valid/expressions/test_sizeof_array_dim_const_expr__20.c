#define N 5
int main(void) {
    int sz = (int)sizeof(int[N * 4]);
    return sz - 60;
}
