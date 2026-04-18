#ifndef CCOMPILER_LEXER_H
#define CCOMPILER_LEXER_H

#include "token.h"

typedef struct {
    const char *source;
    int pos;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next_token(Lexer *lexer);

#endif
