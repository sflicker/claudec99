/* Stage 40: plain "unsigned" is equivalent to "unsigned int". */
int main() {
    unsigned x = 100;
    unsigned y = 58;
    unsigned z = x - y;
    if (z == 42)
        return 42;
    return 1;
}
