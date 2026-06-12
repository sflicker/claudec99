int f(int n) {
    enum { X = n > 0 };
    return X;
}
int main(void) { return f(1); }
