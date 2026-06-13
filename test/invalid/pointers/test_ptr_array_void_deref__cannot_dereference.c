int main() {
    int x;
    void *items[1];

    x = 42;
    items[0] = &x;

    return *items[0]; // REJECT: cannot dereference void *
}
