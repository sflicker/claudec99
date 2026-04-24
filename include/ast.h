#ifndef CCOMPILER_AST_H
#define CCOMPILER_AST_H

#include "type.h"

typedef enum {
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DECL,
    AST_PARAM,
    AST_RETURN_STATEMENT,
    AST_INT_LITERAL,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_IF_STATEMENT,
    AST_WHILE_STATEMENT,
    AST_DO_WHILE_STATEMENT,
    AST_FOR_STATEMENT,
    AST_SWITCH_STATEMENT,
    AST_DEFAULT_SECTION,
    AST_CASE_SECTION,
    AST_BREAK_STATEMENT,
    AST_CONTINUE_STATEMENT,
    AST_GOTO_STATEMENT,
    AST_LABEL_STATEMENT,
    AST_BLOCK,
    AST_EXPRESSION_STMT,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_VAR_REF,
    AST_PREFIX_INC_DEC,
    AST_POSTFIX_INC_DEC,
    AST_FUNCTION_CALL,
    AST_CAST
} ASTNodeType;

#define AST_MAX_CHILDREN 64

typedef struct ASTNode {
    ASTNodeType type;
    char value[256];
    struct ASTNode *children[AST_MAX_CHILDREN];
    int child_count;
    TypeKind decl_type;
    TypeKind result_type;
} ASTNode;

ASTNode *ast_new(ASTNodeType type, const char *value);
void ast_add_child(ASTNode *parent, ASTNode *child);
void ast_free(ASTNode *node);

#endif
