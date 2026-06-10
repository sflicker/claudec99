/* globals_b.c: defines a global array with designated init */
int scores[5] = { [1] = 10, [3] = 30, [0] = 5 };

int total_score(void) {
    int sum = 0;
    int i;
    for (i = 0; i < 5; i = i + 1)
        sum = sum + scores[i];
    return sum;
}
