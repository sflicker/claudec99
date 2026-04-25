int main() {
    int x = 1;
    int *p = &x;
    *p = 2;
    *p = 3;
    return *p;
}
