int *identity(int *p) {
    return p;
}

int main() {
    int a = 1;
    char *p = identity(&a);
    return 0;
}
