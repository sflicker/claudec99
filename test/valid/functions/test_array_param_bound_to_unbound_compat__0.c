/* CC99-008: named bound [3] then unnamed bound [] are compatible array params. */
int first(int a[3]);
int first(int a[]) {
    return a[0];
}

int main(void) {
    int v[3] = { 0, 1, 2 };
    return first(v);
}
