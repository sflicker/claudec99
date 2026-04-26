int read(int *p) {
    return *p;
}

int main() {
    int x = 7;
    return read(&x);
}
