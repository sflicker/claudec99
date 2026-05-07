#ifndef CCOMPILER_PARSER_H
#define CCOMPILER_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"
#include "type.h"
#include <stdio.h>

#define PARSER_MAX_FUNCTIONS 64
#define PARSER_MAX_GLOBALS 64
#define FUNC_MAX_PARAMS 16

typedef struct {
    char name[256];
    int param_count;
    int has_definition;
    TypeKind return_type;
    TypeKind param_types[FUNC_MAX_PARAMS];
    /* Stage 23: linkage of the first declaration. SC_NONE and SC_EXTERN
     * are both external; SC_STATIC is internal. */
    StorageClass storage_class;
} FuncSig;

/* Stage 22-02: tracks each file-scope object declaration so the parser
 * can detect duplicates and function/object name conflicts.
 * Stage 23: storage_class tracks extern/static/none linkage. */
typedef struct {
    char name[256];
    TypeKind kind;
    StorageClass storage_class;
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
