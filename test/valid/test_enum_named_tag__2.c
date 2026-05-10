enum Color {
    RED,
    GREEN,
    BLUE
};

int main() {
    enum Color c = BLUE;
    return c;    /* expect 2 */
}
