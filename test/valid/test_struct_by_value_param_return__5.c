/* Stage 91-01: a register-class (16-byte) struct passed and returned by value.
 * finalize() receives a copy of the Token, sets its length, and returns the
 * modified copy; main assigns it back into t. Expected exit code: 5. */
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

    t.value = "hello";
    t.length = 0;

    t = finalize(t);

    return t.length;    /* expect 5 */
}
