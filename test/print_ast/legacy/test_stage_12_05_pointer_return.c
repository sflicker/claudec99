int *identity(int *p) {
    return p;
}

int **id2(int **pp) {
    return pp;
}

int main() {
    int x = 7;
    int *p = identity(&x);
    return *p;
}
