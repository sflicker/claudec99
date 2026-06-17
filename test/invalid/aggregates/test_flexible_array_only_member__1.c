/* A struct containing only a flexible array member is invalid. */
struct Bad {
    int data[];
};

int main(void) {
    return 0;
}
