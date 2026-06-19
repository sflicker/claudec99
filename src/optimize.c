/*
 * optimize.c - AST-level optimizer pass.
 *
 * Stage 142: infrastructure -- bottom-up tree walk, no transformations.
 * Stage 143: constant integer binary folding and unary ~ folding.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "optimize.h"
#include "ast.h"
#include "util.h"

static ASTNode *optimize_expr(ASTNode *node);
static ASTNode *optimize_stmt(ASTNode *node);

static ASTNode *optimize_expr(ASTNode *node) {
    int i;
    if (node == NULL) return NULL;

    /* Recurse into all children first (bottom-up). */
    for (i = 0; i < node->child_count; i++)
        node->children[i] = optimize_expr(node->children[i]);

    /* Constant integer binary folding. */
    if (node->type == AST_BINARY_OP &&
            node->child_count == 2 &&
            node->children[0]->type == AST_INT_LITERAL &&
            node->children[1]->type == AST_INT_LITERAL) {
        const char *op = node->value;
        long lval = strtol(node->children[0]->value, NULL, 0);
        long rval = strtol(node->children[1]->value, NULL, 0);
        long result = 0;
        int do_fold = 1;
        int result_is_bool = 0;
        TypeKind left_type = node->children[0]->decl_type;
        int left_unsigned = node->children[0]->is_unsigned;
        char buf[32];

        if      (strcmp(op, "+")  == 0) { result = lval + rval; }
        else if (strcmp(op, "-")  == 0) { result = lval - rval; }
        else if (strcmp(op, "*")  == 0) { result = lval * rval; }
        else if (strcmp(op, "/")  == 0) {
            if (rval == 0) { do_fold = 0; }
            else           { result = lval / rval; }
        }
        else if (strcmp(op, "%")  == 0) {
            if (rval == 0) { do_fold = 0; }
            else           { result = lval % rval; }
        }
        else if (strcmp(op, "&")  == 0) { result = lval & rval; }
        else if (strcmp(op, "|")  == 0) { result = lval | rval; }
        else if (strcmp(op, "^")  == 0) { result = lval ^ rval; }
        else if (strcmp(op, "<<") == 0) { result = lval << rval; }
        else if (strcmp(op, ">>") == 0) { result = lval >> rval; }
        else if (strcmp(op, "<")  == 0) { result = lval < rval;  result_is_bool = 1; }
        else if (strcmp(op, "<=") == 0) { result = lval <= rval; result_is_bool = 1; }
        else if (strcmp(op, ">")  == 0) { result = lval > rval;  result_is_bool = 1; }
        else if (strcmp(op, ">=") == 0) { result = lval >= rval; result_is_bool = 1; }
        else if (strcmp(op, "==") == 0) { result = lval == rval; result_is_bool = 1; }
        else if (strcmp(op, "!=") == 0) { result = lval != rval; result_is_bool = 1; }
        else { do_fold = 0; } /* && and || handled separately below */

        if (do_fold) {
            ASTNode *lit;
            snprintf(buf, sizeof(buf), "%ld", result);
            ast_free(node);
            lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type   = result_is_bool ? TYPE_INT : left_type;
            lit->is_unsigned = result_is_bool ? 0        : left_unsigned;
            return lit;
        }
    }

    /* Logical short-circuit folding. */
    if (node->type == AST_BINARY_OP && node->child_count == 2) {
        const char *op       = node->value;
        int left_is_lit      = (node->children[0]->type == AST_INT_LITERAL);
        int right_is_lit     = (node->children[1]->type == AST_INT_LITERAL);

        if (strcmp(op, "&&") == 0 && left_is_lit) {
            long lval = strtol(node->children[0]->value, NULL, 0);
            if (lval == 0) {
                ASTNode *z = ast_new(AST_INT_LITERAL, "0");
                z->decl_type = TYPE_INT;
                ast_free(node);
                return z;
            }
            if (right_is_lit) {
                long rval = strtol(node->children[1]->value, NULL, 0);
                ASTNode *lit = ast_new(AST_INT_LITERAL, rval != 0 ? "1" : "0");
                lit->decl_type = TYPE_INT;
                ast_free(node);
                return lit;
            }
        }

        if (strcmp(op, "||") == 0 && left_is_lit) {
            long lval = strtol(node->children[0]->value, NULL, 0);
            if (lval != 0) {
                ASTNode *one = ast_new(AST_INT_LITERAL, "1");
                one->decl_type = TYPE_INT;
                ast_free(node);
                return one;
            }
            if (right_is_lit) {
                long rval = strtol(node->children[1]->value, NULL, 0);
                ASTNode *lit = ast_new(AST_INT_LITERAL, rval != 0 ? "1" : "0");
                lit->decl_type = TYPE_INT;
                ast_free(node);
                return lit;
            }
        }
    }

    /* Constant unary bitwise-NOT folding. */
    if (node->type == AST_UNARY_OP &&
            node->child_count == 1 &&
            strcmp(node->value, "~") == 0 &&
            node->children[0]->type == AST_INT_LITERAL) {
        long val = strtol(node->children[0]->value, NULL, 0);
        long result = ~val;
        TypeKind operand_type    = node->children[0]->decl_type;
        int      operand_unsigned = node->children[0]->is_unsigned;
        char buf[32];
        ASTNode *lit;
        snprintf(buf, sizeof(buf), "%ld", result);
        ast_free(node);
        lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
        lit->decl_type   = operand_type;
        lit->is_unsigned = operand_unsigned;
        return lit;
    }

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
