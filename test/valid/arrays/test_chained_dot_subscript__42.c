struct Node {
    int values[3];
};

int main(void) {
    struct Node n;

    n.values[0] = 40;
    n.values[1] = 2;

    return n.values[0] + n.values[1];
}
