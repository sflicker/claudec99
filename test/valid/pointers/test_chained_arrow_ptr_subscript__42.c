struct Buffer {
    int *items;
};

int main(void) {
    int values[3];
    struct Buffer b;
    struct Buffer *p;

    values[0] = 10;
    values[1] = 20;
    values[2] = 12;

    b.items = values;
    p = &b;

    return p->items[0] + p->items[1] + p->items[2];
}
