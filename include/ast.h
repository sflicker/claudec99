#ifndef CCOMPILER_AST_H
#define CCOMPILER_AST_H

#include "type.h"

typedef enum {
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DECL,
    AST_PARAM,
    AST_RETURN_STATEMENT,
    AST_INT_LITERAL,
    AST_STRING_LITERAL,
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
    AST_CAST,
    AST_ADDR_OF,
    AST_DEREF,
    AST_ARRAY_INDEX
} ASTNodeType;

#define AST_MAX_CHILDREN 64

typedef struct ASTNode {
    ASTNodeType type;
    char value[256];
    struct ASTNode *children[AST_MAX_CHILDREN];
    int child_count;
    TypeKind decl_type;
    TypeKind result_type;
    /* Stage 12-01: full Type chain for pointer declarations. NULL for
     * non-pointer nodes; for AST_DECLARATION with decl_type ==
     * TYPE_POINTER, points to the head of the pointer Type chain. */
    struct Type *full_type;
    /* Stage 14-05: decoded byte count for AST_STRING_LITERAL. The
     * payload bytes live in `value` but may contain embedded NULs once
     * `\0` escapes are supported, so length must be carried alongside. */
    int byte_length;
} ASTNode;

ASTNode *ast_new(ASTNodeType type, const char *value);
void ast_add_child(ASTNode *parent, ASTNode *child);
void ast_free(ASTNode *node);

#endif
