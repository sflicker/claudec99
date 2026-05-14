typedef struct Node Node;

struct Node {
    int value;
    Node *next;
};

int main() {
    Node a;
    Node b;
    Node *items[2];

    a.value = 10;
    b.value = 20;

    items[0] = &a;
    items[1] = &b;

    return items[0]->value + items[1]->value; // expect 30
}
