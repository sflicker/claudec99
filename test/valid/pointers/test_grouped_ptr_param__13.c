int read(int (*p)) {
    return *p;
}

int main() {
    int x = 13;
    return read(&x);
}
