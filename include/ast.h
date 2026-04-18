#ifndef CCOMPILER_AST_H
#define CCOMPILER_AST_H

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
    AST_FOR_STATEMENT,
    AST_BLOCK,
    AST_EXPRESSION_STMT,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_VAR_REF,
    AST_PREFIX_INC_DEC,
    AST_POSTFIX_INC_DEC
} ASTNodeType;

#define AST_MAX_CHILDREN 64

typedef struct ASTNode {
    ASTNodeType type;
    char value[256];
    struct ASTNode *children[AST_MAX_CHILDREN];
    int child_count;
} ASTNode;

ASTNode *ast_new(ASTNodeType type, const char *value);
void ast_add_child(ASTNode *parent, ASTNode *child);
void ast_free(ASTNode *node);

#endif
