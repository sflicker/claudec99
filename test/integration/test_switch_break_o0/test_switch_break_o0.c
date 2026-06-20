#include <stdio.h>
/* Regression test: switch-based state machine, -O0 baseline.
   Expected output: 23 (GCC and cc99 -O0 agree). */
enum State {
    ST_START  = 0,
    ST_WORD   = 1,
    ST_NUMBER = 2,
    ST_COMMENT = 3,
    ST_ERROR  = 4
};

static char stream[9] = "a*7rh *4";

static int parse_stream(void) {
    enum State st = ST_START;
    int i;
    int score = 0;

    for (i = 0; stream[i]; i = i + 1) {
        char ch = stream[i];
        switch (st) {
        case ST_START:
            if (ch >= 'a' && ch <= 'z') {
                st = ST_WORD;
                score = score + 3;
            } else if (ch >= '0' && ch <= '9') {
                st = ST_NUMBER;
                score = score + 5;
            } else if (ch == '/') {
                st = ST_COMMENT;
                score = score + 7;
            }
            break;
        case ST_WORD:
            if (ch >= 'a' && ch <= 'z') {
                score = score + ch;
            } else {
                st = ST_START;
                score = score ^ i;
            }
            break;
        case ST_NUMBER:
            if (ch >= '0' && ch <= '9') {
                score = score * 3 + ch - '0';
            } else {
                st = ST_START;
                score = score + 11;
            }
            break;
        case ST_COMMENT:
            if (ch == '*') {
                st = ST_START;
                score = score + 13;
            } else if (ch >= '0' && ch <= '9') {
                st = ST_ERROR;
                score = score - ch;
            }
            break;
        default:
            score = score + ch;
            if ((score & 15) == 0) {
                st = ST_START;
            }
            break;
        }
    }

    return score + st;
}

int main(void) {
    printf("%d\n", parse_stream() & 255);
    return 0;
}
