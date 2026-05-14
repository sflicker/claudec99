struct Counter {
    int value;
};

int main() {
    struct Counter a;
    struct Counter b;
    struct Counter *items[2];

    a.value = 1;
    b.value = 2;

    items[0] = &a;
    items[1] = &b;

    items[0]->value = 10;
    items[1]->value = 20;

    return a.value + b.value; // expect 30
}
