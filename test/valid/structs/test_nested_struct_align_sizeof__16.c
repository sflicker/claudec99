struct Inner {
    char c;
    int x;
};

struct Outer {
    char a;
    struct Inner inner;
    char b;
};

int main() {
    return sizeof(struct Outer);
}
