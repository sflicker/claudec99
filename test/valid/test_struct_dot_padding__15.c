struct Mixed {
    char c;
    int i;
};

int main() {
    struct Mixed s;
    s.c = 5;
    s.i = 10;
    return s.c + s.i;
}
