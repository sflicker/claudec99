union U {
    int i;
    char c;
};

int main(void) {
    union U u = {42};
    return u.i;
}
