#include <stdio.h>
#include <stdlib.h>
#include "codegen_arm64.h"
#include "util.h"

/*
 * Stage 181: ARM64 (AArch64) backend skeleton for the arm64-macos target
 * (Phase 1 of docs/outlines/arm64-macos-backend-roadmap.md).
 *
 * Supported input: function definitions whose body is a single
 * `return <integer-literal>;` with a value in 0..65535.  Preamble typedefs
 * and bodiless prototypes are ignored.  Every other construct is rejected
 * with "arm64-macos: <construct> not yet supported"; later phases extend
 * this file construct by construct.
 *
 * Output is Apple/GNU assembler syntax, assembled and linked with
 * `clang -arch arm64`.  Each function gets the full AAPCS64 frame:
 *
 *     stp x29, x30, [sp, #-16]!
 *     mov x29, sp
 *     ...
 *     ldp x29, x30, [sp], #16
 *     ret
 */

/* Short readable name of an AST node type for "not yet supported" errors. */
static const char *arm64_construct_name(ASTNodeType t) {
    switch (t) {
    case AST_TRANSLATION_UNIT:   return "translation unit";
    case AST_FUNCTION_DECL:      return "function declaration";
    case AST_PARAM:              return "function parameter";
    case AST_RETURN_STATEMENT:   return "return statement";
    case AST_INT_LITERAL:        return "integer literal";
    case AST_FLOAT_LITERAL:      return "floating-point literal";
    case AST_STRING_LITERAL:     return "string literal";
    case AST_CHAR_LITERAL:       return "character literal";
    case AST_BINARY_OP:          return "binary expression";
    case AST_UNARY_OP:           return "unary expression";
    case AST_IF_STATEMENT:       return "if statement";
    case AST_WHILE_STATEMENT:    return "while statement";
    case AST_DO_WHILE_STATEMENT: return "do-while statement";
    case AST_FOR_STATEMENT:      return "for statement";
    case AST_SWITCH_STATEMENT:   return "switch statement";
    case AST_DEFAULT_SECTION:    return "default label";
    case AST_CASE_SECTION:       return "case label";
    case AST_BREAK_STATEMENT:    return "break statement";
    case AST_CONTINUE_STATEMENT: return "continue statement";
    case AST_GOTO_STATEMENT:     return "goto statement";
    case AST_LABEL_STATEMENT:    return "labeled statement";
    case AST_BLOCK:              return "compound statement";
    case AST_EXPRESSION_STMT:    return "expression statement";
    case AST_DECLARATION:        return "variable declaration";
    case AST_ASSIGNMENT:         return "assignment";
    case AST_VAR_REF:            return "variable reference";
    case AST_PREFIX_INC_DEC:     return "prefix increment/decrement";
    case AST_POSTFIX_INC_DEC:    return "postfix increment/decrement";
    case AST_FUNCTION_CALL:      return "function call";
    case AST_INDIRECT_CALL:      return "indirect function call";
    case AST_CAST:               return "cast";
    case AST_ADDR_OF:            return "address-of";
    case AST_DEREF:              return "dereference";
    case AST_ARRAY_INDEX:        return "array subscript";
    case AST_SIZEOF_TYPE:        return "sizeof";
    case AST_SIZEOF_EXPR:        return "sizeof";
    case AST_CONDITIONAL_EXPR:   return "conditional expression";
    case AST_COMMA_EXPR:         return "comma expression";
    case AST_DECL_LIST:          return "declaration list";
    case AST_TYPEDEF_DECL:       return "typedef";
    case AST_MEMBER_ACCESS:      return "member access";
    case AST_ARROW_ACCESS:       return "member access";
    case AST_INITIALIZER_LIST:   return "initializer list";
    case AST_DESIGNATED_INIT:    return "designated initializer";
    case AST_COMPOUND_LITERAL:   return "compound literal";
    case AST_BUILTIN_VA_START:   return "va_start";
    case AST_BUILTIN_VA_END:     return "va_end";
    case AST_BUILTIN_VA_COPY:    return "va_copy";
    case AST_BUILTIN_VA_ARG:     return "va_arg";
    }
    return "construct";
}

/* Record a node's source position for compile_error() (mirrors cg_mark in
 * codegen.c). */
static void arm64_mark(const ASTNode *node) {
    if (node && node->src_line > 0) {
        g_error_src_file = node->src_file;
        g_error_src_line = node->src_line;
        g_error_src_col  = node->src_col;
    }
}

static void arm64_unsupported(ASTNode *node, const char *what) {
    arm64_mark(node);
    compile_error("error: arm64-macos: %s not yet supported\n", what);
}

void codegen_arm64_init(CodeGenArm64 *cg, FILE *output) {
    cg->output = output;
}

void codegen_arm64_free(CodeGenArm64 *cg) {
    (void)cg;
}

/* Load an integer literal into w0 (Phase 1: 16-bit immediates only). */
static void arm64_expr_to_w0(CodeGenArm64 *cg, ASTNode *expr) {
    long v;
    if (expr->type != AST_INT_LITERAL) {
        arm64_unsupported(expr, arm64_construct_name(expr->type));
    }
    v = strtol(expr->value, NULL, 0);
    if (v < 0 || v > 65535) {
        arm64_mark(expr);
        compile_error("error: arm64-macos: integer literal %ld out of range "
                      "for this stage\n", v);
    }
    fprintf(cg->output, "\tmov\tw0, #%ld\n", v);
}

static ASTNode *arm64_function_body(ASTNode *func) {
    int i;
    for (i = func->child_count - 1; i >= 0; i--) {
        if (func->children[i]->type == AST_BLOCK) return func->children[i];
    }
    return NULL;
}

static void arm64_function(CodeGenArm64 *cg, ASTNode *func, ASTNode *body) {
    int i;
    ASTNode *ret;

    /* Parameters are not supported yet (an empty `()`/`(void)` list
     * produces no AST_PARAM children). */
    for (i = 0; i < func->child_count; i++) {
        if (func->children[i]->type == AST_PARAM)
            arm64_unsupported(func->children[i], "function parameter");
    }
    /* Report the first statement that is not a return, by construct. */
    for (i = 0; i < body->child_count; i++) {
        ASTNode *s = body->children[i];
        if (s->type == AST_DECLARATION)
            arm64_unsupported(s, "local variable declaration");
        if (s->type != AST_RETURN_STATEMENT)
            arm64_unsupported(s, arm64_construct_name(s->type));
    }
    if (body->child_count != 1)
        arm64_unsupported(body, body->child_count == 0
                                ? "empty function body"
                                : "multi-statement function body");
    ret = body->children[0];
    if (ret->child_count != 1)
        arm64_unsupported(ret, "return without a value");

    if (func->storage_class != SC_STATIC)
        fprintf(cg->output, "\t.globl\t_%s\n", func->value);
    fprintf(cg->output, "\t.p2align\t2\n");
    fprintf(cg->output, "_%s:\n", func->value);
    fprintf(cg->output, "\tstp\tx29, x30, [sp, #-16]!\n");
    fprintf(cg->output, "\tmov\tx29, sp\n");
    arm64_expr_to_w0(cg, ret->children[0]);
    fprintf(cg->output, "\tldp\tx29, x30, [sp], #16\n");
    fprintf(cg->output, "\tret\n");
}

void codegen_arm64_translation_unit(CodeGenArm64 *cg, ASTNode *tu) {
    int i;
    fprintf(cg->output, "\t.section\t__TEXT,__text,regular,pure_instructions\n");
    for (i = 0; i < tu->child_count; i++) {
        ASTNode *n = tu->children[i];
        ASTNode *body;
        if (n->type == AST_TYPEDEF_DECL) continue;
        if (n->type != AST_FUNCTION_DECL) {
            arm64_unsupported(n, n->type == AST_DECLARATION
                                 ? "global variable"
                                 : arm64_construct_name(n->type));
        }
        body = arm64_function_body(n);
        if (!body) continue;   /* prototype */
        arm64_function(cg, n, body);
    }
}
