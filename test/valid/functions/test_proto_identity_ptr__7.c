int *identity(int *p);

int main() {
    int x = 7;
    int *p = identity(&x);
    return *p;
}

int *identity(int *p) {
    return p;
}
