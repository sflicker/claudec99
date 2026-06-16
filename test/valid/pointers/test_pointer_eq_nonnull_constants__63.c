/*
 * Expected conforming return code: 63.
 */
int main(void) {
    int local = 0;
    int *local_ptr = &local;
    int *one = (int *)1;
    void *two = (void *)2;
    int score = 0;

    if (one == (int *)1) {
        score = score + 1;
    }

    if (one != (int *)2) {
        score = score + 2;
    }

    if ((int *)0 != one) {
        score = score + 4;
    }

    if (two == (void *)2) {
        score = score + 8;
    }

    if ((char *)3 != (char *)4) {
        score = score + 16;
    }

    if (local_ptr != (int *)1) {
        score = score + 32;
    }

    return score;    /* expect 63 */
}
