typedef struct Node Node;
struct Node {
    int value;
    Node *next;
};

int main() {
    Node n;
    n.value = 7;
    n.next = &n;
    return n.next->value;
}
