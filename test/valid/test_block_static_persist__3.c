int next(void) {
    static int x;
    x = x + 1;
    return x;
}

int main() {
    return next() + next();    // expect 3
}
