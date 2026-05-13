/* Stage 40: unsigned remainder. */
int main() {
    unsigned int a = 7;
    unsigned int b = 3;
    unsigned int r = a % b;
    if (r == 1)
        return 42;
    return 1;
}
