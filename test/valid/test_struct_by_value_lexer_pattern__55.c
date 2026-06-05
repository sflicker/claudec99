/* Stage 91-01: the lexer/parser Token pattern that blocked self-compilation —
 * a large (memory-class) struct with a char-array member, returned by value
 * from a function taking a pointer argument, plus a nested struct-by-value
 * call (relabel(make(...))) and struct decl-initialization from a call.
 * Expected exit code: 55. */
#include <stdio.h>
#include <string.h>

typedef struct Token {
    int type;
    char value[32];
    int length;
    long extra;
} Token;

typedef struct Lexer {
    int pos;
} Lexer;

static Token make(Lexer *lx, int type) {
    Token t;
    t.type = type;
    t.length = lx->pos;
    strcpy(t.value, "hi");
    t.extra = 7;
    lx->pos = lx->pos + 1;
    return t;
}

static Token relabel(Token t, int newtype) {
    t.type = newtype;
    return t;
}

int main(void) {
    Lexer lx;
    lx.pos = 0;

    Token a = make(&lx, 5);            /* decl-init from a struct-returning call */
    Token b;
    b = relabel(make(&lx, 9), 99);     /* nested struct-by-value call */

    if (a.type != 5) return 81;
    if (a.length != 0) return 82;
    if (strcmp(a.value, "hi") != 0) return 83;
    if (a.extra != 7) return 84;
    if (b.type != 99) return 85;
    if (b.length != 1) return 86;
    if (lx.pos != 2) return 87;
    return 55;
}
