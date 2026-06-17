/* CC99-009: pointer-to-array parameter: int (*row)[] compatible with int (*row)[4].
 * The composite type rule uses the known bound; (*row)[i] indexes correctly. */
int tail(int (*row)[]);

int tail(int (*row)[4]) {
    return (*row)[3];
}

int main(void) {
    int row[4] = { 1, 2, 3, 13 };
    return tail(&row);
}
