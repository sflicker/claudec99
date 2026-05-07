#ifndef CCOMPILER_PARSER_H
#define CCOMPILER_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"
#include "type.h"

#define PARSER_MAX_FUNCTIONS 64
#define PARSER_MAX_GLOBALS 64
#define FUNC_MAX_PARAMS 16

typedef struct {
    char name[256];
    int param_count;
    int has_definition;
    TypeKind return_type;
    TypeKind param_types[FUNC_MAX_PARAMS];
} FuncSig;

/* Stage 22-02: tracks each file-scope object declaration so the parser
 * can detect duplicates and function/object name conflicts. */
typedef struct {
    char name[256];
    TypeKind kind;
} GlobalObjSig;

typedef struct {
    Lexer *lexer;
    Token current;
    FuncSig funcs[PARSER_MAX_FUNCTIONS];
    int func_count;
    /* Stage 22-02: file-scope object table. */
    GlobalObjSig globals[PARSER_MAX_GLOBALS];
    int global_count;
    int loop_depth;
    int switch_depth;
} Parser;

void parser_init(Parser *parser, Lexer *lexer);
ASTNode *parse_translation_unit(Parser *parser);

#endif
