/*
 * sizeof result type test.
 *
 * In C99, sizeof yields size_t, an unsigned integer type. These checks detect
 * compilers that incorrectly lower sizeof as a signed int.
 *
 * Expected conforming return code: 6.
 */
int main(void) {
    int score = 0;

    if (sizeof(int) > -1) {
        score = score + 1;
    }

    if (sizeof(char) - 2 > 0) {
        score = score + 2;
    }

    if ((sizeof(int) < 0) == 0) {
        score = score + 4;
    }

    return score;
}
