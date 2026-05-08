int add(int x, int y) {
    return x + y;
}

int main() {
    int (*op)(int, int) = add;
    return op(2, 3);   /* expect 5 */
}
