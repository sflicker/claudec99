int read2(int **pp) {
    return **pp;
}

int main() {
    int x = 11;
    int *p = &x;
    return read2(&p);
}
