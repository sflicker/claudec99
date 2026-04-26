int read(int *p) {
    return *p;
}

int main() {
    char c = 7;
    return read(&c);
}
