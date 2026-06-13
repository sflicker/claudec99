union U {
    int i;
    char c;
};

int main() {
    union U u;
    union U *p;

    p = &u;
    p->i = 42;

    return u.i;
}
