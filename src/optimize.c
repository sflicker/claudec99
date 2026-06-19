/*
 * optimize.c - AST-level optimizer pass.
 *
 * Stage 142: infrastructure only.  All helpers are no-ops that traverse
 * the tree bottom-up and return every node unchanged.  Future stages add
 * transformation rules by inserting early-return replacements at the top
 * of each case.
 */

#include <stddef.h>
#include "optimize.h"
#include "ast.h"

static ASTNode *optimize_expr(ASTNode *node);
static ASTNode *optimize_stmt(ASTNode *node);

static ASTNode *optimize_expr(ASTNode *node) {
    int i;
    if (node == NULL) return NULL;

    /* Recurse into all children first (bottom-up). */
    for (i = 0; i < node->child_count; i++)
        node->children[i] = optimize_expr(node->children[i]);

    /*
     * Future stages insert transformation rules here, keyed on node->type.
     * Example structure (not yet active):
     *   if (node->type == AST_BINARY_OP) { ... return replacement; }
     */

    return node;
}

static ASTNode *optimize_stmt(ASTNode *node) {
    int i;
    if (node == NULL) return NULL;

    switch (node->type) {

    case AST_BLOCK:
        for (i = 0; i < node->child_count; i++)
            node->children[i] = optimize_stmt(node->children[i]);
        return node;

    case AST_IF_STATEMENT:
        /* children: [condition, then-body, (optional) else-body] */
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        if (node->child_count > 2)
            node->children[2] = optimize_stmt(node->children[2]);
        return node;

    case AST_WHILE_STATEMENT:
        /* children: [condition, body] */
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        return node;

    case AST_DO_WHILE_STATEMENT:
        /* children: [body, condition] */
        node->children[0] = optimize_stmt(node->children[0]);
        node->children[1] = optimize_expr(node->children[1]);
        return node;

    case AST_FOR_STATEMENT:
        /* children: [init|NULL, cond|NULL, update|NULL, body] */
        if (node->children[0]) node->children[0] = optimize_stmt(node->children[0]);
        if (node->children[1]) node->children[1] = optimize_expr(node->children[1]);
        if (node->children[2]) node->children[2] = optimize_expr(node->children[2]);
        node->children[3] = optimize_stmt(node->children[3]);
        return node;

    case AST_SWITCH_STATEMENT:
        /* children: [discriminant, body] */
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        return node;

    case AST_RETURN_STATEMENT:
        if (node->child_count > 0)
            node->children[0] = optimize_expr(node->children[0]);
        return node;

    case AST_EXPRESSION_STMT:
        if (node->child_count > 0)
            node->children[0] = optimize_expr(node->children[0]);
        return node;

    case AST_DECLARATION:
    case AST_DECL_LIST:
        /* Initializer expressions are children of DECLARATION nodes. */
        for (i = 0; i < node->child_count; i++)
            node->children[i] = optimize_expr(node->children[i]);
        return node;

    case AST_LABEL_STATEMENT:
    case AST_CASE_SECTION:
    case AST_DEFAULT_SECTION:
        /* children: [body-statement, ...] */
        for (i = 0; i < node->child_count; i++)
            node->children[i] = optimize_stmt(node->children[i]);
        return node;

    case AST_BREAK_STATEMENT:
    case AST_CONTINUE_STATEMENT:
    case AST_GOTO_STATEMENT:
        return node; /* no children to optimize */

    default:
        /* Any node type not listed above -- recurse generically. */
        for (i = 0; i < node->child_count; i++)
            node->children[i] = optimize_expr(node->children[i]);
        return node;
    }
}

ASTNode *optimize_translation_unit(ASTNode *root, int opt_level) {
    int i;
    if (opt_level == 0) return root;

    /* root is AST_TRANSLATION_UNIT; children are top-level declarations
     * and function definitions. */
    for (i = 0; i < root->child_count; i++) {
        ASTNode *decl = root->children[i];
        if (decl->type == AST_FUNCTION_DECL) {
            /* Last child of AST_FUNCTION_DECL is the body block (if this
             * is a definition, not a prototype).  Prototypes have no body;
             * detect by checking whether the last child is AST_BLOCK. */
            int last = decl->child_count - 1;
            if (last >= 0 && decl->children[last]->type == AST_BLOCK)
                decl->children[last] = optimize_stmt(decl->children[last]);
        }
    }
    return root;
}
