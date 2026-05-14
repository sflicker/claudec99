int sum_first_two(int **items) {
    return *items[0] + *items[1];
}

int main() {
    int a;
    int b;
    int *items[2];

    a = 13;
    b = 17;

    items[0] = &a;
    items[1] = &b;

    return sum_first_two(items); // expect 30
}
