#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

ASTNode *ast_new(ASTNodeType type, const char *value) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "error: out of memory\n");
        exit(1);
    }
    node->type = type;
    if (value) {
        strncpy(node->value, value, sizeof(node->value) - 1);
    }
    return node;
}

void ast_add_child(ASTNode *parent, ASTNode *child) {
    if (parent->child_count < AST_MAX_CHILDREN) {
        parent->children[parent->child_count++] = child;
    }
}

void ast_free(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        ast_free(node->children[i]);
    }
    free(node);
}
