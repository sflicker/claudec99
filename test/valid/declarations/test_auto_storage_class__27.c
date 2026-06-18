int compute(int seed) {
    auto int value = seed + 3;
    auto int values[3] = { 2, 4, 6 };
    auto int i;
    auto int total = value;

    for (i = 0; i < 3; i = i + 1) {
        total = total + values[i];
    }

    {
        auto int inner = total + 5;
        total = inner;
    }

    return total;
}

int main(void) {
    return compute(7);
}
