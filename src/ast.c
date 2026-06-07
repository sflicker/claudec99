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
    node->value = value;
    return node;
}

void ast_add_child(ASTNode *parent, ASTNode *child) {
    if (parent->child_count >= parent->child_cap) {
        int new_cap = parent->child_cap ? parent->child_cap * 2 : AST_MAX_CHILDREN;
        ASTNode **grown = realloc(parent->children,
                                  (size_t)new_cap * sizeof(ASTNode *));
        if (!grown) {
            fprintf(stderr, "error: out of memory\n");
            exit(1);
        }
        parent->children = grown;
        parent->child_cap = new_cap;
    }
    parent->children[parent->child_count++] = child;
}

/*
 * Stage 79: deep-copy an AST subtree. Used to desugar compound assignment
 * on a general lvalue (`lhs op= rhs` => `lhs = lhs op rhs`), where the lvalue
 * must appear both as the assignment target and as the left operand of the
 * binary op. Cloning avoids sharing the same node pointer in two places.
 *
 * The full_type Type chain is persistent/shared, so the pointer is copied
 * directly rather than deep-copied.
 */
ASTNode *ast_clone(ASTNode *node) {
    if (!node) return NULL;
    ASTNode *copy = ast_new(node->type, node->value);
    copy->decl_type = node->decl_type;
    copy->result_type = node->result_type;
    copy->full_type = node->full_type;
    copy->byte_length = node->byte_length;
    copy->storage_class = node->storage_class;
    copy->is_const = node->is_const;
    copy->is_unsigned = node->is_unsigned;
    copy->is_variadic = node->is_variadic;
    copy->src_file = node->src_file;
    copy->src_line = node->src_line;
    copy->src_col = node->src_col;
    for (int i = 0; i < node->child_count; i++) {
        ast_add_child(copy, ast_clone(node->children[i]));
    }
    return copy;
}

void ast_free(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        ast_free(node->children[i]);
    }
    free(node->children);
    free(node);
}
