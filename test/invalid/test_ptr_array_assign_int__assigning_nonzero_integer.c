int main() {
    int *items[2];

    items[0] = 123; // REJECT: assigning nonzero integer to pointer
    return 0;
}
