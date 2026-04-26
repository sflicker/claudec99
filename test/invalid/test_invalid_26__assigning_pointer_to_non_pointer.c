int *identity(int *p) {
    return p;
}

int main() {
    int a = 1;
    int b = identity(&a);
    return b;
}
