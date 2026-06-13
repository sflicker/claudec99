union U {
    char c;
    int i;
    long l;
};

int main() {
    union U u;
    return sizeof(u);
}
