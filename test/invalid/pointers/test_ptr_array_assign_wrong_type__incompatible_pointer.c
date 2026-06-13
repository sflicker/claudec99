int main() {
    int x;
    char *items[1];

    items[0] = &x; // REJECT: int * assigned to char *
    return 0;
}
