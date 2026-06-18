int compute(register int seed) {
    register int value = seed + 3;
    register int i;
    int values[3] = { 2, 4, 6 };
    register int total = value;

    for (i = 0; i < 3; i = i + 1) {
        total = total + values[i];
    }

    {
        register int inner = total + 5;
        total = inner;
    }

    return total;
}

int main(void) {
    return compute(7);
}
