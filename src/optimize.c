/*
 * optimize.c - AST-level optimizer pass.
 *
 * Stage 142: infrastructure -- bottom-up tree walk, no transformations.
 * Stage 143: constant integer binary folding and unary ~ folding.
 * Stage 146: strength reduction -- x*2^N -> x<<N, x/2^N -> x>>N (unsigned).
 * Stage 147: boolean/logical simplification -- !!x, x&&0, x||1, x&&1, x||0.
 * Stage 148: negation folding -- -(-x) -> x for non-constant x.
 * Stage 149: conditional expression folding -- const ? T : F -> selected branch.
 * Stage 150: dead-branch elimination -- if/while/for with constant-zero condition.
 * Stage 151: sizeof constant folding -- AST_SIZEOF_TYPE/EXPR -> AST_INT_LITERAL.
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

/* Map a scalar TypeKind to its sizeof value.
   Mirrors codegen_expression's AST_SIZEOF_TYPE scalar switch, with one fix:
   TYPE_DOUBLE is returned as 8 (codegen falls through to default:4, a bug). */
static int sizeof_scalar_size(TypeKind k) {
    switch (k) {
    case TYPE_BOOL:               return 1;
    case TYPE_CHAR:               return 1;
    case TYPE_SHORT:              return 2;
    case TYPE_LONG:               return 8;
    case TYPE_LONG_LONG:          return 8;
    case TYPE_UNSIGNED_LONG_LONG: return 8;
    case TYPE_POINTER:            return 8;
    case TYPE_DOUBLE:             return 8;
    default:                      return 4; /* TYPE_INT, TYPE_FLOAT */
    }
}

/* Create an AST_INT_LITERAL whose value and type match what sizeof returns.
   sizeof always yields unsigned size_t (C99 §6.5.3.4); we represent that as
   TYPE_LONG / is_unsigned=1 to match what codegen sets on sizeof nodes. */
static ASTNode *make_sizeof_literal(int sz) {
    char buf[16];
    ASTNode *lit;
    snprintf(buf, sizeof(buf), "%d", sz);
    lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
    lit->decl_type = TYPE_LONG;
    lit->is_unsigned = 1;
    return lit;
}

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

    /* Constant unary folding: -, +, !, ~ */
    if (node->type == AST_UNARY_OP &&
            node->child_count == 1 &&
            node->children[0]->type == AST_INT_LITERAL) {
        const char *op           = node->value;
        long val                 = strtol(node->children[0]->value, NULL, 0);
        long result              = 0;
        int  is_logical          = 0;
        int  do_fold             = 1;
        TypeKind operand_type    = node->children[0]->decl_type;
        int  operand_unsigned    = node->children[0]->is_unsigned;
        char buf[32];
        ASTNode *lit;

        if      (strcmp(op, "-") == 0) { result = -val; }
        else if (strcmp(op, "+") == 0) { result = val; }
        else if (strcmp(op, "!") == 0) { result = (val == 0) ? 1 : 0; is_logical = 1; }
        else if (strcmp(op, "~") == 0) { result = ~val; }
        else                           { do_fold = 0; }

        if (do_fold) {
            snprintf(buf, sizeof(buf), "%ld", result);
            ast_free(node);
            lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type   = is_logical ? TYPE_INT : operand_type;
            lit->is_unsigned = is_logical ? 0        : operand_unsigned;
            return lit;
        }
    }

    /* Double logical NOT: !!x -> (x != 0) for non-constant x.
       When x is a literal, stage-144 already folds both ! applications;
       this rule fires only when the inner !x child is not an INT_LITERAL. */
    if (node->type == AST_UNARY_OP &&
            node->child_count == 1 &&
            strcmp(node->value, "!") == 0) {
        ASTNode *inner = node->children[0];
        ASTNode *x;
        ASTNode *neq;
        ASTNode *zero;
        int fire;

        fire = (inner != NULL &&
                inner->type == AST_UNARY_OP &&
                inner->child_count == 1 &&
                strcmp(inner->value, "!") == 0 &&
                inner->children[0] != NULL &&
                inner->children[0]->type != AST_INT_LITERAL);

        if (fire) {
            x = inner->children[0];
            inner->children[0] = NULL; /* prevent double-free of x */
            ast_free(node);            /* frees outer ! and inner ! (not x) */
            neq = ast_new(AST_BINARY_OP, util_strdup("!="));
            neq->decl_type = TYPE_INT;
            zero = ast_new(AST_INT_LITERAL, "0");
            zero->decl_type = TYPE_INT;
            ast_add_child(neq, x);
            ast_add_child(neq, zero);
            return neq;
        }
    }

    /* Double arithmetic negation: -(-x) -> x for non-constant x.
       When x is a literal, stage-144 folds both - applications in two passes;
       this rule fires only when the innermost child is not an INT_LITERAL. */
    if (node->type == AST_UNARY_OP &&
            node->child_count == 1 &&
            strcmp(node->value, "-") == 0) {
        ASTNode *inner = node->children[0];
        ASTNode *x;
        int fire;

        fire = (inner != NULL &&
                inner->type == AST_UNARY_OP &&
                inner->child_count == 1 &&
                strcmp(inner->value, "-") == 0 &&
                inner->children[0] != NULL &&
                inner->children[0]->type != AST_INT_LITERAL);

        if (fire) {
            x = inner->children[0];
            inner->children[0] = NULL; /* prevent double-free of x */
            ast_free(node);            /* frees outer - and inner - (not x) */
            return x;
        }
    }

    /* Algebraic identity folding. */
    if (node->type == AST_BINARY_OP && node->child_count == 2) {
        const char *op        = node->value;
        ASTNode    *left      = node->children[0];
        ASTNode    *right     = node->children[1];
        int left_is_lit       = (left->type  == AST_INT_LITERAL);
        int right_is_lit      = (right->type == AST_INT_LITERAL);
        long lval             = left_is_lit  ? strtol(left->value,  NULL, 0) : 0L;
        long rval             = right_is_lit ? strtol(right->value, NULL, 0) : 0L;
        int left_is_zero      = left_is_lit  && (lval == 0L);
        int right_is_zero     = right_is_lit && (rval == 0L);
        int left_is_one       = left_is_lit  && (lval == 1L);
        int right_is_one      = right_is_lit && (rval == 1L);
        int left_is_allones   = left_is_lit  && (lval == -1L);
        int right_is_allones  = right_is_lit && (rval == -1L);
        int both_same_var     = (left->type  == AST_VAR_REF &&
                                 right->type == AST_VAR_REF &&
                                 strcmp(left->value, right->value) == 0);
        ASTNode *z;

        /* Identity rules: result is one existing child.
           Null the kept child's slot before ast_free to avoid double-free. */
        if (strcmp(op, "+") == 0 && right_is_zero)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "+") == 0 && left_is_zero)
            { node->children[1] = NULL; ast_free(node); return right; }
        if (strcmp(op, "-") == 0 && right_is_zero)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "*") == 0 && right_is_one)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "*") == 0 && left_is_one)
            { node->children[1] = NULL; ast_free(node); return right; }
        if (strcmp(op, "/") == 0 && right_is_one)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "|") == 0 && right_is_zero)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "|") == 0 && left_is_zero)
            { node->children[1] = NULL; ast_free(node); return right; }
        if (strcmp(op, "&") == 0 && right_is_allones)
            { node->children[0] = NULL; ast_free(node); return left; }
        if (strcmp(op, "&") == 0 && left_is_allones)
            { node->children[1] = NULL; ast_free(node); return right; }

        /* Zero rules: free entire subtree and return a fresh zero literal. */
        z = NULL;
        if      (strcmp(op, "*") == 0 && right_is_zero)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = left->decl_type;  z->is_unsigned = left->is_unsigned;  }
        else if (strcmp(op, "*") == 0 && left_is_zero)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = right->decl_type; z->is_unsigned = right->is_unsigned; }
        else if (strcmp(op, "/") == 0 && left_is_zero)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = right->decl_type; z->is_unsigned = right->is_unsigned; }
        else if (strcmp(op, "&") == 0 && right_is_zero)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = left->decl_type;  z->is_unsigned = left->is_unsigned;  }
        else if (strcmp(op, "&") == 0 && left_is_zero)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = right->decl_type; z->is_unsigned = right->is_unsigned; }
        else if (strcmp(op, "-") == 0 && both_same_var)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = left->decl_type;  z->is_unsigned = left->is_unsigned;  }
        else if (strcmp(op, "^") == 0 && both_same_var)
            { z = ast_new(AST_INT_LITERAL, "0"); z->decl_type = left->decl_type;  z->is_unsigned = left->is_unsigned;  }
        if (z != NULL) { ast_free(node); return z; }
    }

    /* Strength reduction: x * 2^N -> x << N; x / 2^N -> x >> N (unsigned). */
    if (node->type == AST_BINARY_OP && node->child_count == 2) {
        const char *op    = node->value;
        ASTNode    *left  = node->children[0];
        ASTNode    *right = node->children[1];
        int  do_mul       = (strcmp(op, "*") == 0);
        int  do_div       = (strcmp(op, "/") == 0);
        int  right_is_lit = (right->type == AST_INT_LITERAL);
        int  left_is_lit  = (left->type  == AST_INT_LITERAL);
        long rval         = right_is_lit ? strtol(right->value, NULL, 0) : 0L;
        long lval         = left_is_lit  ? strtol(left->value,  NULL, 0) : 0L;
        int  r_is_pow2    = right_is_lit && (rval > 1L) && ((rval & (rval - 1L)) == 0L);
        int  l_is_pow2    = left_is_lit  && (lval > 1L) && ((lval & (lval - 1L)) == 0L);
        int  r_shift      = 0;
        int  l_shift      = 0;
        int  left_nonneg  = left_is_lit  && (lval >= 0L);
        long tmp;
        ASTNode *lit;
        char buf[16];

        if (r_is_pow2) {
            tmp = rval;
            while (tmp > 1L) { tmp >>= 1; r_shift++; }
        }
        if (l_is_pow2) {
            tmp = lval;
            while (tmp > 1L) { tmp >>= 1; l_shift++; }
        }

        /* x * 2^N -> x << N  (right operand is power of two) */
        if (do_mul && r_is_pow2) {
            snprintf(buf, sizeof(buf), "%d", r_shift);
            lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type   = TYPE_INT;
            lit->is_unsigned = 0;
            ast_free(right);
            node->children[1] = lit;
            node->value = util_strdup("<<");
            return node;
        }

        /* 2^N * x -> x << N  (left operand is power of two; move x to left slot) */
        if (do_mul && l_is_pow2) {
            snprintf(buf, sizeof(buf), "%d", l_shift);
            lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type   = TYPE_INT;
            lit->is_unsigned = 0;
            node->children[0] = right;  /* x moves to left slot  */
            node->children[1] = lit;
            ast_free(left);             /* free the 2^N literal  */
            node->value = util_strdup("<<");
            return node;
        }

        /* x / 2^N -> x >> N  (unsigned dividend or statically non-negative literal) */
        if (do_div && r_is_pow2 && (left->is_unsigned || left_nonneg)) {
            snprintf(buf, sizeof(buf), "%d", r_shift);
            lit = ast_new(AST_INT_LITERAL, util_strdup(buf));
            lit->decl_type   = TYPE_INT;
            lit->is_unsigned = 0;
            ast_free(right);
            node->children[1] = lit;
            node->value = util_strdup(">>");
            return node;
        }
    }

    /* Boolean/logical simplification: x&&0, x||nonzero, x&&nonzero, x||0.
       Cases where the LEFT operand is a literal are already handled by the
       logical short-circuit block (stage 143); this block handles a nonzero
       or zero RIGHT constant. */
    if (node->type == AST_BINARY_OP && node->child_count == 2) {
        const char *op       = node->value;
        ASTNode    *right    = node->children[1];
        int right_is_lit     = (right->type == AST_INT_LITERAL);
        long rval            = right_is_lit ? strtol(right->value, NULL, 0) : 0L;
        int right_is_zero    = right_is_lit && (rval == 0L);
        int right_is_nonzero = right_is_lit && (rval != 0L);
        ASTNode *z;
        ASTNode *zero_lit;

        /* x && 0 -> 0 */
        if (strcmp(op, "&&") == 0 && right_is_zero) {
            z = ast_new(AST_INT_LITERAL, "0");
            z->decl_type = TYPE_INT;
            ast_free(node);
            return z;
        }

        /* x || nonzero -> 1 */
        if (strcmp(op, "||") == 0 && right_is_nonzero) {
            z = ast_new(AST_INT_LITERAL, "1");
            z->decl_type = TYPE_INT;
            ast_free(node);
            return z;
        }

        /* x && nonzero -> (x != 0): replace right child with 0, change op */
        if (strcmp(op, "&&") == 0 && right_is_nonzero) {
            zero_lit = ast_new(AST_INT_LITERAL, "0");
            zero_lit->decl_type = TYPE_INT;
            ast_free(right);
            node->children[1] = zero_lit;
            node->value = util_strdup("!=");
            node->decl_type = TYPE_INT;
            return node;
        }

        /* x || 0 -> (x != 0): right child is already 0, just change operator */
        if (strcmp(op, "||") == 0 && right_is_zero) {
            node->value = util_strdup("!=");
            node->decl_type = TYPE_INT;
            return node;
        }
    }

    /* Conditional expression folding: const ? T : F -> selected branch.
       Bottom-up walk ensures the condition has already been folded; if it
       reduced to a literal we eliminate the dead branch entirely. */
    if (node->type == AST_CONDITIONAL_EXPR &&
            node->child_count == 3 &&
            node->children[0] != NULL &&
            node->children[0]->type == AST_INT_LITERAL) {
        long cval     = strtol(node->children[0]->value, NULL, 0);
        int  keep_idx = (cval != 0L) ? 1 : 2;
        ASTNode *keep = node->children[keep_idx];
        node->children[keep_idx] = NULL; /* detach before ast_free */
        ast_free(node);                  /* frees ?: node, condition, dead branch */
        return keep;
    }

    /* sizeof(type) folding: AST_SIZEOF_TYPE is always a compile-time constant.
       All type information (decl_type, full_type) is set by the parser. */
    if (node->type == AST_SIZEOF_TYPE) {
        int sz;
        if ((node->decl_type == TYPE_STRUCT || node->decl_type == TYPE_UNION ||
                node->decl_type == TYPE_ARRAY) && node->full_type) {
            sz = node->full_type->size;
        } else {
            sz = sizeof_scalar_size(node->decl_type);
        }
        ast_free(node);
        return make_sizeof_literal(sz);
    }

    /* sizeof(expr) folding for operands whose size the optimizer can determine
       without symbol-table access.  All other cases fall through to codegen. */
    if (node->type == AST_SIZEOF_EXPR && node->child_count == 1 &&
            node->children[0] != NULL) {
        ASTNode *child = node->children[0];
        /* sizeof("literal") = byte_length + 1 (includes null terminator). */
        if (child->type == AST_STRING_LITERAL) {
            int sz = child->byte_length + 1;
            ast_free(node);
            return make_sizeof_literal(sz);
        }
        /* sizeof(integer-literal): size from suffix-determined decl_type. */
        if (child->type == AST_INT_LITERAL) {
            int sz = sizeof_scalar_size(child->decl_type);
            ast_free(node);
            return make_sizeof_literal(sz);
        }
        /* sizeof(char-literal): character constants have type int in C99. */
        if (child->type == AST_CHAR_LITERAL) {
            ast_free(node);
            return make_sizeof_literal(4);
        }
        /* Variable references and complex expressions: leave for codegen. */
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

    case AST_IF_STATEMENT: {
        /* children: [condition, then-body, (optional) else-body] */
        ASTNode *keep;
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        if (node->child_count > 2)
            node->children[2] = optimize_stmt(node->children[2]);
        if (node->children[0]->type == AST_INT_LITERAL) {
            long cval = strtol(node->children[0]->value, NULL, 0);
            if (cval != 0L) {
                /* Always true: keep then-branch, drop else-branch. */
                keep = node->children[1];
                node->children[1] = NULL;
                ast_free(node);
                return keep;
            } else {
                /* Always false: keep else-branch (or empty block). */
                keep = (node->child_count > 2)
                       ? node->children[2]
                       : ast_new(AST_BLOCK, NULL);
                if (node->child_count > 2) node->children[2] = NULL;
                ast_free(node);
                return keep;
            }
        }
        return node;
    }

    case AST_WHILE_STATEMENT:
        /* children: [condition, body] */
        node->children[0] = optimize_expr(node->children[0]);
        node->children[1] = optimize_stmt(node->children[1]);
        if (node->children[0]->type == AST_INT_LITERAL &&
                strtol(node->children[0]->value, NULL, 0) == 0L) {
            ast_free(node);
            return ast_new(AST_BLOCK, NULL);
        }
        return node;

    case AST_DO_WHILE_STATEMENT:
        /* children: [body, condition] */
        node->children[0] = optimize_stmt(node->children[0]);
        node->children[1] = optimize_expr(node->children[1]);
        return node;

    case AST_FOR_STATEMENT: {
        /* children: [init|NULL, cond|NULL, update|NULL, body] */
        ASTNode *init;
        if (node->children[0]) node->children[0] = optimize_stmt(node->children[0]);
        if (node->children[1]) node->children[1] = optimize_expr(node->children[1]);
        if (node->children[2]) node->children[2] = optimize_expr(node->children[2]);
        node->children[3] = optimize_stmt(node->children[3]);
        if (node->children[1] != NULL &&
                node->children[1]->type == AST_INT_LITERAL &&
                strtol(node->children[1]->value, NULL, 0) == 0L) {
            ASTNode *stmt;
            init = node->children[0];
            node->children[0] = NULL; /* detach init before ast_free */
            ast_free(node);           /* frees for-node, cond, update, body */
            if (init == NULL) return ast_new(AST_BLOCK, NULL);
            /* Declarations are already statement nodes; expression inits
               must be wrapped so codegen_statement can emit them. */
            if (init->type == AST_DECLARATION || init->type == AST_DECL_LIST)
                return init;
            stmt = ast_new(AST_EXPRESSION_STMT, NULL);
            ast_add_child(stmt, init);
            return stmt;
        }
        return node;
    }

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
