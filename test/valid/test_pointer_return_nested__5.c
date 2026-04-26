int **id2(int **pp) {
    return pp;
}

int main() {
    int x = 5;
    int *p = &x;
    int **pp = &p;
    return **id2(pp);
}
