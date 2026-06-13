/* Stage 91-01: prove by-value semantics. finalize() mutates only its private
 * copy, so the caller's original t is unchanged (length still 99) while the
 * returned copy u has the computed length 5. Expected exit code: 1. */
#include <stdio.h>
#include <string.h>

typedef struct Token {
    char *value;
    int length;
} Token;

static Token finalize(Token token) {
    token.length = (int)strlen(token.value);
    return token;
}

int main(void) {
    Token t;
    Token u;

    t.value = "hello";
    t.length = 99;

    u = finalize(t);

    return t.length == 99 && u.length == 5;    /* expect 1 */
}
