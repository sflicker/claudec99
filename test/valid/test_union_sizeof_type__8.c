union U {
    char c;
    int i;
    long l;
};

int main() {
    return sizeof(union U);
}
