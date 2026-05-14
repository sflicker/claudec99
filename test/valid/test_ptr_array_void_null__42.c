int main() {
    void *items[2];

    items[0] = 0;
    items[1] = 0;

    if (items[0] == items[1])
        return 42;

    return 1;
}
