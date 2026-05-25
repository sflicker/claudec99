union U {
    int i;
    char c;
};

union U u;

int main(void) {
    u.i = 42;
    return u.i;
}
