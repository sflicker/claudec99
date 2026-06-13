union U {
    int i;
    char c;
};

int main() {
    union U u;
    u.i = 42;
    return u.i;
}
