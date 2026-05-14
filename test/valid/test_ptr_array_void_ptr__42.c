int main() {
    int x;
    int *p;
    void *items[2];

    x = 42;

    items[0] = &x;  // int * -> void * OK
    p = items[0];   // void * -> int * OK

    return *p; // expect 42
}
