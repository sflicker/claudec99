int read(int *p);

int read2(int **pp) {
    return **pp;
}

int main() {
    int x = 7;
    return read(&x);
}
