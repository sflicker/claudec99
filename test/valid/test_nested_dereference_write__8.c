int main() {
    int x = 2;
    int *p = &x;
    int **pp = &p;
    **pp = 8;
    return x;
}
