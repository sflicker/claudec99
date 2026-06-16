/* Tests C99 §6.5.2.2p6: default argument promotions for calls through
 * a declaration with no prototype information `int f()`. */

int check();

int check(int c, int s, int us, double f, double d) {
    int score = 0;

    if (c == 65)                    score += 1;   /* char -> int */
    if (s == -1234)                 score += 2;   /* short -> int */
    if (us == 60000)                score += 4;   /* unsigned short -> int */
    if ((int)(f * 10.0) == 15)     score += 8;   /* float -> double */
    if ((int)(d * 10.0) == 25)     score += 16;  /* double stays double */

    return score;
}

int main(void) {
    char           c  = 65;
    short          s  = -1234;
    unsigned short us = 60000;
    float          f  = 1.5f;
    double         d  = 2.5;

    return check(c, s, us, f, d);
}
