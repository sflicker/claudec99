/* CC99-008: int a[3], int *a, and int a[] are all compatible parameter forms. */
int sum_edges(int values[3]);
int sum_edges(int *values);

int sum_edges(int values[]) {
    return values[0] + values[2];
}

int main(void) {
    int values[3] = { 2, 5, 7 };
    return sum_edges(values);
}
