struct Counter {
    int value;
};

int inc(struct Counter *c) {
    c->value = c->value + 1;
    return 0;
}

int main() {
    struct Counter c;
    c.value = 40;
    inc(&c);
    inc(&c);
    return c.value;  /* expect 42 */
}
