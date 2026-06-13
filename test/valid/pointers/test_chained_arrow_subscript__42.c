struct Node {
    int values[3];
};

int main(void) {
    struct Node n;
    struct Node *p;

    p = &n;
    p->values[0] = 10;
    p->values[1] = 20;
    p->values[2] = 12;

    return p->values[0] + p->values[1] + p->values[2];
}
