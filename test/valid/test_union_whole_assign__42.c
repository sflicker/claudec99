union U {
    int i;
    char c;
};

int main(void) {
    union U a;
    union U b;

    a.i = 42;
    b = a;
    return b.i;
}
