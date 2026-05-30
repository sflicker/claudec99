struct Parser {
    int pos;
    int values[4];
};

int main(void) {
    struct Parser p;

    p.pos = 3;
    p.values[3] = 42;

    return p.values[p.pos];
}
