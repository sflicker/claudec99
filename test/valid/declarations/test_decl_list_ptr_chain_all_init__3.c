int main() {
    int a = 3, *p = &a, **q = &p;
    return **q;
}
