/* Stage 159: anonymous struct/union members parse without error. */
struct Color {
    int tag;
    union {
        int rgba;
        struct { unsigned char r, g, b, a; } parts;
    };
};

int main(void) {
    struct Color c;
    c.tag = 7;
    if (c.tag != 7) return 1;
    return 42;
}
