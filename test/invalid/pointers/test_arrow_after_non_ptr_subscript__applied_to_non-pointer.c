struct S {
    int x;
};

int main(void) {
    struct S xs[2];
    return xs[0]->x;
}
