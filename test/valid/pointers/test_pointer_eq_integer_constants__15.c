/*
 * Expected extension return code: 15.
 */
int main(void) {
    int *one = (int *)1;
    int *two = (int *)2;
    int score = 0;

    if (one == 1) {
        score = score + 1;
    }

    if (one != 2) {
        score = score + 2;
    }

    if (two == 2) {
        score = score + 4;
    }

    if (two != 1) {
        score = score + 8;
    }

    return score;    /* expected 15 */
}
