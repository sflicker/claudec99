#ifndef CCOMPILER_PARSER_H
#define CCOMPILER_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"

#define PARSER_MAX_FUNCTIONS 64

typedef struct {
    char name[256];
    int param_count;
} FuncSig;

typedef struct {
    Lexer *lexer;
    Token current;
    FuncSig funcs[PARSER_MAX_FUNCTIONS];
    int func_count;
} Parser;

void parser_init(Parser *parser, Lexer *lexer);
ASTNode *parse_translation_unit(Parser *parser);

#endif
