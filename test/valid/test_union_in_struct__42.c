union U {
    int i;
    char c;
};

struct Box {
    int kind;
    union U value;
};

int main() {
    struct Box b;
    b.kind = 1;
    b.value.i = 41;
    return b.kind + b.value.i;
}
