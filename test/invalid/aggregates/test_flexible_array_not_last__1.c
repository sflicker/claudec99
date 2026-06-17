/* A flexible array member that is not the last named member is invalid. */
struct Bad {
    int count;
    int data[];
    int extra;
};

int main(void) {
    return 0;
}
