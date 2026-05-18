#define VALUE 42
int main() {
    int x = VALUE;
#undef VALUE
#ifdef VALUE
    x += 10;
#endif
    return x;
}
