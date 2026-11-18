#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen_arm64.h"
#include "util.h"

/*
 * Stage 181: ARM64 (AArch64) backend skeleton for the arm64-macos target
 * (Phase 1 of docs/outlines/arm64-macos-backend-roadmap.md).
 *
 * Stage 182 (Phase 2): straight-line signed integer code.  Supported:
 * block-scope int / long / long long locals (with or without an
 * initializer, nested blocks with shadowing), return and expression
 * statements, + - * / % & | ^ << >>, unary - ~ +, assignment to a local
 * (compound assignment arrives lowered by the parser), prefix/postfix
 * ++/--, the comma operator, and integer literals of any size.  Every
 * other construct is rejected with "arm64-macos: <construct> not yet
 * supported"; later phases extend this file construct by construct.
 *
 * Stage 183 (Phase 3): comparisons (cmp + cset), && || ! (short-circuit
 * via cbz/cbnz), ?:, if/else, while, do-while, for, break, continue,
 * switch/case/default and goto/labels.  Internal labels are
 * L<n>_<kind>, numbered by a translation-unit-wide counter; user labels
 * are L<func>_usr_<name>.  At every statement boundary sp equals
 * x29 - frame_size, so every jump is a plain branch.
 *
 * Value model: every expression leaves its result in w0 (int) or x0
 * (long); arm64_expr_is_long() gives an expression's width.  Binary
 * operators use a stack machine: the left operand is pushed with
 * `str x0, [sp, #-16]!` (keeping sp 16-byte aligned), the right operand
 * is evaluated and moved to x1, and the left is popped back into x0.
 *
 * Output is Apple/GNU assembler syntax, assembled and linked with
 * `clang -arch arm64`.  Each function gets the full AAPCS64 frame:
 *
 *     stp x29, x30, [sp, #-16]!
 *     mov x29, sp
 *     sub sp, sp, #<frame>          (omitted when there are no locals)
 *     ...
 * L<func>_epilogue:
 *     mov sp, x29
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

/* Stage 182: readable name of a type kind, used to build "<kind> type"
 * and "<kind> return type" errors. */
static const char *arm64_type_name(TypeKind k) {
    switch (k) {
    case TYPE_VOID:               return "void";
    case TYPE_BOOL:               return "_Bool";
    case TYPE_CHAR:               return "char";
    case TYPE_SHORT:              return "short";
    case TYPE_INT:                return "int";
    case TYPE_LONG:               return "long";
    case TYPE_LONG_LONG:          return "long long";
    case TYPE_UNSIGNED_LONG_LONG: return "unsigned";
    case TYPE_FLOAT:              return "floating-point";
    case TYPE_DOUBLE:             return "floating-point";
    case TYPE_POINTER:            return "pointer";
    case TYPE_ARRAY:              return "array";
    case TYPE_FUNCTION:           return "function";
    case TYPE_STRUCT:             return "struct";
    case TYPE_UNION:              return "union";
    }
    return "unknown";
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

/* Stage 182: reject a non-int/long type with "<kind><suffix> not yet
 * supported" (suffix is " type" or " return type"). */
static void arm64_unsupported_type(ASTNode *node, TypeKind k, int is_unsigned,
                                   const char *suffix) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%s",
             is_unsigned ? "unsigned" : arm64_type_name(k), suffix);
    arm64_unsupported(node, buf);
}

/* Stage 182: 1 if a kind is a 64-bit signed integer type. */
static int arm64_kind_is_long(TypeKind k) {
    return k == TYPE_LONG || k == TYPE_LONG_LONG;
}

void codegen_arm64_init(CodeGenArm64 *cg, FILE *output) {
    cg->output = output;
    vec_init(&cg->locals, sizeof(Arm64Local));
    cg->scope_depth = 0;
    cg->next_offset = 0;
    cg->frame_size = 0;
    cg->func_name = NULL;
    cg->ret_is_long = 0;
    cg->label_count = 0;
    vec_init(&cg->jumps, sizeof(Arm64Jump));
    vec_init(&cg->switches, sizeof(Arm64Switch));
    vec_init(&cg->user_labels, sizeof(const char *));
}

void codegen_arm64_free(CodeGenArm64 *cg) {
    size_t i;
    vec_free(&cg->locals);
    vec_free(&cg->jumps);
    for (i = 0; i < cg->switches.len; i++) {
        Arm64Switch *sw = (Arm64Switch *)vec_get(&cg->switches, i);
        vec_free(&sw->entries);
    }
    vec_free(&cg->switches);
    vec_free(&cg->user_labels);
}

/* ------------------------------------------------------------------ */
/* Locals and scopes                                                   */
/* ------------------------------------------------------------------ */

/* Innermost-first lookup, so an inner declaration shadows an outer one. */
static Arm64Local *arm64_find_local(CodeGenArm64 *cg, const char *name) {
    size_t i = cg->locals.len;
    while (i > 0) {
        Arm64Local *l;
        i--;
        l = (Arm64Local *)vec_get(&cg->locals, i);
        if (strcmp(l->name, name) == 0) return l;
    }
    return NULL;
}

/* Look up a variable reference that must name a local. */
static Arm64Local *arm64_require_local(CodeGenArm64 *cg, ASTNode *node,
                                       const char *name) {
    Arm64Local *l = arm64_find_local(cg, name);
    if (!l) arm64_unsupported(node, "global variable");
    return l;
}

/* Leaving a block drops the locals it declared. */
static void arm64_pop_scope(CodeGenArm64 *cg) {
    while (cg->locals.len > 0) {
        Arm64Local *l = (Arm64Local *)vec_get(&cg->locals,
                                              cg->locals.len - 1);
        if (l->scope_depth < cg->scope_depth) break;
        vec_pop(&cg->locals);
    }
    cg->scope_depth--;
}

/* Hand out the next frame slot: 4 bytes at 4-byte alignment for int,
 * 8 bytes at 8-byte alignment for long.  Returns the slot's offset below
 * x29.  The pre-scan and the emitter call this in the same order, so they
 * agree on the layout; slots are never reused. */
static int arm64_alloc_slot(int *next_offset, int is_long) {
    int size = is_long ? 8 : 4;
    *next_offset = (*next_offset + size + size - 1) / size * size;
    return *next_offset;
}

/* Pre-scan: size every declaration in a statement tree.  Stage 183:
 * descends every non-NULL child (if/else branches, loop bodies, for
 * init, case/default/label bodies, switch bodies), in child order, which
 * is the emitter's visiting order.  Expressions never hold declarations
 * (compound literals are rejected), so descending them is harmless. */
static void arm64_prescan(ASTNode *node, int *next_offset) {
    int i;
    if (!node) return;
    if (node->type == AST_DECLARATION) {
        arm64_alloc_slot(next_offset, arm64_kind_is_long(node->decl_type));
        return;
    }
    for (i = 0; i < node->child_count; i++)
        arm64_prescan(node->children[i], next_offset);
}

/* Emit a load or store of w0/x0 (or w1/x1) to a frame slot.  ldr/str with
 * a negative offset assemble as ldur/stur, whose range is -256..255;
 * deeper slots go through x9. */
static void arm64_slot_access(CodeGenArm64 *cg, const char *op, int reg,
                              int is_long, int offset) {
    char r = is_long ? 'x' : 'w';
    if (offset <= 256) {
        fprintf(cg->output, "\t%s\t%c%d, [x29, #-%d]\n", op, r, reg, offset);
    } else {
        fprintf(cg->output, "\tsub\tx9, x29, #%d\n", offset);
        fprintf(cg->output, "\t%s\t%c%d, [x9]\n", op, r, reg);
    }
}

/* ------------------------------------------------------------------ */
/* Expressions                                                         */
/* ------------------------------------------------------------------ */

/* Name of the assignment target: the parser's identifier form stores it
 * in node->value (one child, the rhs); the general-lvalue form has
 * children (lhs, rhs), and only a variable is accepted here. */
static const char *arm64_assign_target(ASTNode *e, ASTNode **rhs) {
    if (e->value) {
        *rhs = e->children[0];
        return e->value;
    }
    *rhs = e->children[1];
    if (e->children[0]->type != AST_VAR_REF)
        arm64_unsupported(e->children[0],
                          arm64_construct_name(e->children[0]->type));
    return e->children[0]->value;
}

static int arm64_is_shift(const char *op) {
    return strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0;
}

/* Stage 183: condition code of a comparison operator, or NULL. */
static const char *arm64_compare_cc(const char *op) {
    if (strcmp(op, "==") == 0) return "eq";
    if (strcmp(op, "!=") == 0) return "ne";
    if (strcmp(op, "<") == 0)  return "lt";
    if (strcmp(op, "<=") == 0) return "le";
    if (strcmp(op, ">") == 0)  return "gt";
    if (strcmp(op, ">=") == 0) return "ge";
    return NULL;
}

static int arm64_is_logical(const char *op) {
    return strcmp(op, "&&") == 0 || strcmp(op, "||") == 0;
}

/* Pure width helper: 1 if the expression's value is 64-bit. */
static int arm64_expr_is_long(CodeGenArm64 *cg, ASTNode *e) {
    Arm64Local *l;
    ASTNode *rhs;
    switch (e->type) {
    case AST_INT_LITERAL:
        return arm64_kind_is_long(e->decl_type);
    case AST_VAR_REF:
        l = arm64_find_local(cg, e->value);
        return l ? l->is_long : 0;
    case AST_BINARY_OP:
        /* Stage 183: comparisons and && || yield int. */
        if (arm64_compare_cc(e->value) || arm64_is_logical(e->value))
            return 0;
        if (arm64_is_shift(e->value))
            return arm64_expr_is_long(cg, e->children[0]);
        return arm64_expr_is_long(cg, e->children[0]) ||
               arm64_expr_is_long(cg, e->children[1]);
    case AST_UNARY_OP:
        if (strcmp(e->value, "!") == 0) return 0;
        return arm64_expr_is_long(cg, e->children[0]);
    case AST_CONDITIONAL_EXPR:
        return arm64_expr_is_long(cg, e->children[1]) ||
               arm64_expr_is_long(cg, e->children[2]);
    case AST_ASSIGNMENT:
        if (!e->value && e->children[0]->type != AST_VAR_REF) return 0;
        l = arm64_find_local(cg, arm64_assign_target(e, &rhs));
        return l ? l->is_long : 0;
    case AST_PREFIX_INC_DEC:
    case AST_POSTFIX_INC_DEC:
        return arm64_expr_is_long(cg, e->children[0]);
    case AST_COMMA_EXPR:
        return arm64_expr_is_long(cg, e->children[e->child_count - 1]);
    default:
        return 0;
    }
}

/* Load a 64-bit bit pattern into w<reg> (low 32 bits) or x<reg>: `mov`
 * for 0..65535, otherwise movz + movk for each further non-zero chunk. */
static void arm64_emit_load_imm(CodeGenArm64 *cg, int reg, unsigned long bits,
                                int is_long) {
    char r = is_long ? 'x' : 'w';
    int top = is_long ? 48 : 16;
    int shift;
    if (!is_long) bits &= 0xFFFFFFFFUL;
    if (bits <= 65535UL) {
        fprintf(cg->output, "\tmov\t%c%d, #%lu\n", r, reg, bits);
        return;
    }
    fprintf(cg->output, "\tmovz\t%c%d, #%lu\n", r, reg, bits & 0xFFFFUL);
    for (shift = 16; shift <= top; shift += 16) {
        unsigned long chunk = (bits >> shift) & 0xFFFFUL;
        if (chunk)
            fprintf(cg->output, "\tmovk\t%c%d, #%lu, lsl #%d\n",
                    r, reg, chunk, shift);
    }
}

static void arm64_expr(CodeGenArm64 *cg, ASTNode *e);

/* Evaluate e and convert it to the requested width (int -> long is
 * `sxtw`; long -> int just uses w0). */
static void arm64_expr_as(CodeGenArm64 *cg, ASTNode *e, int want_long) {
    arm64_expr(cg, e);
    if (want_long && !arm64_expr_is_long(cg, e))
        fprintf(cg->output, "\tsxtw\tx0, w0\n");
}

/* Stage 183: evaluate e and branch to L<n>_<kind> with `insn` (cbz or
 * cbnz), testing w0 or x0 by e's own width. */
static void arm64_cond_branch(CodeGenArm64 *cg, ASTNode *e, const char *insn,
                              int n, const char *kind) {
    arm64_expr(cg, e);
    fprintf(cg->output, "\t%s\t%c0, L%d_%s\n", insn,
            arm64_expr_is_long(cg, e) ? 'x' : 'w', n, kind);
}

/* Stage 183: a < b etc.  Both operands at the common width, then
 * cmp + cset (signed condition codes). */
static void arm64_compare(CodeGenArm64 *cg, ASTNode *e, const char *cc) {
    int is_long = arm64_expr_is_long(cg, e->children[0]) ||
                  arm64_expr_is_long(cg, e->children[1]);
    char r = is_long ? 'x' : 'w';
    arm64_expr_as(cg, e->children[0], is_long);
    fprintf(cg->output, "\tstr\tx0, [sp, #-16]!\n");
    arm64_expr_as(cg, e->children[1], is_long);
    fprintf(cg->output, "\tmov\tx1, x0\n");
    fprintf(cg->output, "\tldr\tx0, [sp], #16\n");
    fprintf(cg->output, "\tcmp\t%c0, %c1\n", r, r);
    fprintf(cg->output, "\tcset\tw0, %s\n", cc);
}

/* Stage 183: short-circuit a && b / a || b; the result is int 0 or 1. */
static void arm64_logical(CodeGenArm64 *cg, ASTNode *e) {
    int n = cg->label_count++;
    int is_and = strcmp(e->value, "&&") == 0;
    const char *insn = is_and ? "cbz" : "cbnz";
    const char *kind = is_and ? "false" : "true";
    arm64_cond_branch(cg, e->children[0], insn, n, kind);
    arm64_cond_branch(cg, e->children[1], insn, n, kind);
    fprintf(cg->output, "\tmov\tw0, #%d\n", is_and ? 1 : 0);
    fprintf(cg->output, "\tb\tL%d_end\n", n);
    fprintf(cg->output, "L%d_%s:\n", n, kind);
    fprintf(cg->output, "\tmov\tw0, #%d\n", is_and ? 0 : 1);
    fprintf(cg->output, "L%d_end:\n", n);
}

/* Stage 183: c ? a : b.  Only the selected branch runs; each branch is
 * converted to the result width before the join. */
static void arm64_conditional(CodeGenArm64 *cg, ASTNode *e) {
    int n = cg->label_count++;
    int is_long = arm64_expr_is_long(cg, e);
    arm64_cond_branch(cg, e->children[0], "cbz", n, "else");
    arm64_expr_as(cg, e->children[1], is_long);
    fprintf(cg->output, "\tb\tL%d_end\n", n);
    fprintf(cg->output, "L%d_else:\n", n);
    arm64_expr_as(cg, e->children[2], is_long);
    fprintf(cg->output, "L%d_end:\n", n);
}

static void arm64_binary(CodeGenArm64 *cg, ASTNode *e) {
    const char *op = e->value;
    int is_long = arm64_expr_is_long(cg, e);
    char r = is_long ? 'x' : 'w';
    const char *insn = NULL;
    const char *cc = arm64_compare_cc(op);

    if (cc) {
        arm64_compare(cg, e, cc);
        return;
    }
    if (arm64_is_logical(op)) {
        arm64_logical(cg, e);
        return;
    }

    if      (strcmp(op, "+") == 0)  insn = "add";
    else if (strcmp(op, "-") == 0)  insn = "sub";
    else if (strcmp(op, "*") == 0)  insn = "mul";
    else if (strcmp(op, "/") == 0)  insn = "sdiv";
    else if (strcmp(op, "&") == 0)  insn = "and";
    else if (strcmp(op, "|") == 0)  insn = "orr";
    else if (strcmp(op, "^") == 0)  insn = "eor";
    else if (strcmp(op, "<<") == 0) insn = "lsl";
    else if (strcmp(op, ">>") == 0) insn = "asr";
    else if (strcmp(op, "%") != 0)
        arm64_unsupported(e, arm64_construct_name(e->type));

    /* Left operand in the operation width, pushed. */
    arm64_expr_as(cg, e->children[0], is_long);
    fprintf(cg->output, "\tstr\tx0, [sp, #-16]!\n");
    /* Right operand: converted to the operation width, except a shift
     * count, which keeps its own width (only its low bits matter). */
    if (insn && (strcmp(insn, "lsl") == 0 || strcmp(insn, "asr") == 0))
        arm64_expr(cg, e->children[1]);
    else
        arm64_expr_as(cg, e->children[1], is_long);
    fprintf(cg->output, "\tmov\tx1, x0\n");
    fprintf(cg->output, "\tldr\tx0, [sp], #16\n");

    if (insn) {
        fprintf(cg->output, "\t%s\t%c0, %c0, %c1\n", insn, r, r, r);
    } else {
        /* % : r0 - (r0 / r1) * r1 */
        fprintf(cg->output, "\tsdiv\t%c2, %c0, %c1\n", r, r, r);
        fprintf(cg->output, "\tmsub\t%c0, %c2, %c1, %c0\n", r, r, r, r);
    }
}

static void arm64_unary(CodeGenArm64 *cg, ASTNode *e) {
    const char *op = e->value;
    char r;
    if (strcmp(op, "!") == 0) {
        /* Stage 183: !a is (a == 0), tested at a's full width. */
        arm64_expr(cg, e->children[0]);
        fprintf(cg->output, "\tcmp\t%c0, #0\n",
                arm64_expr_is_long(cg, e->children[0]) ? 'x' : 'w');
        fprintf(cg->output, "\tcset\tw0, eq\n");
        return;
    }
    arm64_expr(cg, e->children[0]);
    r = arm64_expr_is_long(cg, e) ? 'x' : 'w';
    if (strcmp(op, "-") == 0)
        fprintf(cg->output, "\tneg\t%c0, %c0\n", r, r);
    else if (strcmp(op, "~") == 0)
        fprintf(cg->output, "\tmvn\t%c0, %c0\n", r, r);
    /* unary + : no code */
}

static void arm64_inc_dec(CodeGenArm64 *cg, ASTNode *e) {
    ASTNode *target = e->children[0];
    Arm64Local *l;
    const char *insn = strcmp(e->value, "++") == 0 ? "add" : "sub";
    char r;
    if (target->type != AST_VAR_REF)
        arm64_unsupported(target, arm64_construct_name(target->type));
    l = arm64_require_local(cg, target, target->value);
    r = l->is_long ? 'x' : 'w';
    arm64_slot_access(cg, "ldr", 0, l->is_long, l->offset);
    if (e->type == AST_PREFIX_INC_DEC) {
        /* Result is the new value. */
        fprintf(cg->output, "\t%s\t%c0, %c0, #1\n", insn, r, r);
        arm64_slot_access(cg, "str", 0, l->is_long, l->offset);
    } else {
        /* Result is the old value, left in r0. */
        fprintf(cg->output, "\t%s\t%c1, %c0, #1\n", insn, r, r);
        arm64_slot_access(cg, "str", 1, l->is_long, l->offset);
    }
}

static void arm64_expr(CodeGenArm64 *cg, ASTNode *e) {
    Arm64Local *l;
    ASTNode *rhs;
    const char *name;
    arm64_mark(e);
    switch (e->type) {
    case AST_INT_LITERAL:
        if (e->is_unsigned)
            arm64_unsupported(e, "unsigned integer literal");
        arm64_emit_load_imm(cg, 0, strtoul(e->value, NULL, 0),
                            arm64_kind_is_long(e->decl_type));
        return;
    case AST_VAR_REF:
        l = arm64_require_local(cg, e, e->value);
        arm64_slot_access(cg, "ldr", 0, l->is_long, l->offset);
        return;
    case AST_BINARY_OP:
        arm64_binary(cg, e);
        return;
    case AST_UNARY_OP:
        arm64_unary(cg, e);
        return;
    case AST_ASSIGNMENT:
        name = arm64_assign_target(e, &rhs);
        l = arm64_require_local(cg, e, name);
        arm64_expr_as(cg, rhs, l->is_long);
        arm64_slot_access(cg, "str", 0, l->is_long, l->offset);
        return;
    case AST_PREFIX_INC_DEC:
    case AST_POSTFIX_INC_DEC:
        arm64_inc_dec(cg, e);
        return;
    case AST_COMMA_EXPR:
        arm64_expr(cg, e->children[0]);
        arm64_expr(cg, e->children[1]);
        return;
    case AST_CONDITIONAL_EXPR:
        arm64_conditional(cg, e);
        return;
    default:
        arm64_unsupported(e, arm64_construct_name(e->type));
    }
}

/* ------------------------------------------------------------------ */
/* Statements                                                          */
/* ------------------------------------------------------------------ */

static void arm64_declaration(CodeGenArm64 *cg, ASTNode *d) {
    Arm64Local local;
    arm64_mark(d);
    if (d->storage_class & SC_STATIC)
        arm64_unsupported(d, "static local variable");
    if (d->storage_class & SC_EXTERN)
        arm64_unsupported(d, "extern declaration");
    if ((d->decl_type != TYPE_INT && !arm64_kind_is_long(d->decl_type)) ||
        d->is_unsigned)
        arm64_unsupported_type(d, d->decl_type, d->is_unsigned, " type");

    local.name = d->value;
    local.is_long = arm64_kind_is_long(d->decl_type);
    local.offset = arm64_alloc_slot(&cg->next_offset, local.is_long);
    local.scope_depth = cg->scope_depth;
    /* The declarator is in scope from the end of the declarator, so it
     * is visible in its own initializer (C99 6.2.1p7). */
    vec_push(&cg->locals, &local);
    if (d->child_count > 0) {
        arm64_expr_as(cg, d->children[0], local.is_long);
        arm64_slot_access(cg, "str", 0, local.is_long, local.offset);
    }
}

static void arm64_statement(CodeGenArm64 *cg, ASTNode *s);

/* Stage 183: push a break/continue target (continue_label -1 = switch). */
static void arm64_push_jump(CodeGenArm64 *cg, int break_label,
                            int continue_label) {
    Arm64Jump j;
    j.break_label = break_label;
    j.continue_label = continue_label;
    vec_push(&cg->jumps, &j);
}

/* Stage 183: loop body, with n as its break/continue target. */
static void arm64_loop_body(CodeGenArm64 *cg, ASTNode *body, int n) {
    arm64_push_jump(cg, n, n);
    arm64_statement(cg, body);
    vec_pop(&cg->jumps);
}

/* Stage 183: collect every case/default label of a switch body in source
 * order, without descending into nested switches (their labels belong to
 * them).  Mirrors collect_switch_labels in codegen.c. */
static void arm64_collect_switch_labels(CodeGenArm64 *cg, ASTNode *node,
                                        Arm64Switch *sw) {
    int i;
    Arm64SwitchLabel entry;
    if (!node) return;
    if (node->type == AST_SWITCH_STATEMENT) return;
    if (node->type == AST_CASE_SECTION || node->type == AST_DEFAULT_SECTION) {
        entry.node = node;
        entry.label = cg->label_count++;
        vec_push(&sw->entries, &entry);
        if (node->type == AST_DEFAULT_SECTION) {
            sw->default_label = entry.label;
            if (node->child_count > 0)
                arm64_collect_switch_labels(cg, node->children[0], sw);
        } else if (node->child_count > 1) {
            arm64_collect_switch_labels(cg, node->children[1], sw);
        }
        return;
    }
    for (i = 0; i < node->child_count; i++)
        arm64_collect_switch_labels(cg, node->children[i], sw);
}

/* Stage 183: label number of a case/default node in the innermost
 * switch. */
static int arm64_switch_label(CodeGenArm64 *cg, ASTNode *node) {
    Arm64Switch *sw = (Arm64Switch *)vec_get(&cg->switches,
                                             cg->switches.len - 1);
    size_t i;
    for (i = 0; i < sw->entries.len; i++) {
        Arm64SwitchLabel *e = (Arm64SwitchLabel *)vec_get(&sw->entries, i);
        if (e->node == node) return e->label;
    }
    arm64_unsupported(node, arm64_construct_name(node->type));
    return -1;
}

/* Stage 183: switch.  The discriminant is evaluated into w0/x0 and
 * dispatched immediately by a linear compare-and-branch chain; it is
 * never spilled, so nothing stays on the stack across the body. */
static void arm64_switch(CodeGenArm64 *cg, ASTNode *s) {
    int n = cg->label_count++;
    int is_long = arm64_expr_is_long(cg, s->children[0]);
    char r = is_long ? 'x' : 'w';
    Arm64Switch sw;
    Arm64Switch *top;
    size_t i;

    vec_init(&sw.entries, sizeof(Arm64SwitchLabel));
    sw.default_label = -1;
    arm64_collect_switch_labels(cg, s->children[1], &sw);

    arm64_expr(cg, s->children[0]);
    for (i = 0; i < sw.entries.len; i++) {
        Arm64SwitchLabel *e = (Arm64SwitchLabel *)vec_get(&sw.entries, i);
        unsigned long bits;
        if (e->node->type != AST_CASE_SECTION) continue;
        /* Case values convert to the promoted controlling type
         * (C99 6.8.4.2p5): an int discriminant compares 32 bits. */
        bits = (unsigned long)strtol(e->node->children[0]->value, NULL, 0);
        if (!is_long) bits &= 0xFFFFFFFFUL;
        if (bits <= 4095UL) {
            fprintf(cg->output, "\tcmp\t%c0, #%lu\n", r, bits);
        } else {
            arm64_emit_load_imm(cg, 1, bits, is_long);
            fprintf(cg->output, "\tcmp\t%c0, %c1\n", r, r);
        }
        fprintf(cg->output, "\tb.eq\tL%d_case\n", e->label);
    }
    if (sw.default_label >= 0)
        fprintf(cg->output, "\tb\tL%d_case\n", sw.default_label);
    else
        fprintf(cg->output, "\tb\tL%d_switch_end\n", n);

    /* The Vec header is bit-copied onto the stack; the copy owns it. */
    vec_push(&cg->switches, &sw);
    arm64_push_jump(cg, n, -1);
    arm64_statement(cg, s->children[1]);
    vec_pop(&cg->jumps);
    /* Re-fetch: nested switches may have reallocated the stack. */
    top = (Arm64Switch *)vec_get(&cg->switches, cg->switches.len - 1);
    vec_free(&top->entries);
    vec_pop(&cg->switches);
    fprintf(cg->output, "L%d_switch_end:\n", n);
}

/* Stage 183: pre-walk a function body for user labels, rejecting
 * duplicates (message matches the x86 backend). */
static void arm64_collect_user_labels(CodeGenArm64 *cg, ASTNode *node) {
    int i;
    size_t j;
    if (!node) return;
    if (node->type == AST_LABEL_STATEMENT) {
        for (j = 0; j < cg->user_labels.len; j++) {
            const char **pp = (const char **)vec_get(&cg->user_labels, j);
            if (strcmp(*pp, node->value) == 0) {
                arm64_mark(node);
                compile_error("error: duplicate label '%s' in function '%s'\n",
                              node->value, cg->func_name);
            }
        }
        vec_push(&cg->user_labels, &node->value);
    }
    for (i = 0; i < node->child_count; i++)
        arm64_collect_user_labels(cg, node->children[i]);
}

static int arm64_user_label_defined(CodeGenArm64 *cg, const char *name) {
    size_t j;
    for (j = 0; j < cg->user_labels.len; j++) {
        const char **pp = (const char **)vec_get(&cg->user_labels, j);
        if (strcmp(*pp, name) == 0) return 1;
    }
    return 0;
}

static void arm64_statement(CodeGenArm64 *cg, ASTNode *s) {
    int i;
    int n;
    size_t j;
    Arm64Jump *jp;
    arm64_mark(s);
    switch (s->type) {
    case AST_IF_STATEMENT:
        n = cg->label_count++;
        if (s->child_count == 3) {
            arm64_cond_branch(cg, s->children[0], "cbz", n, "else");
            arm64_statement(cg, s->children[1]);
            fprintf(cg->output, "\tb\tL%d_end\n", n);
            fprintf(cg->output, "L%d_else:\n", n);
            arm64_statement(cg, s->children[2]);
        } else {
            arm64_cond_branch(cg, s->children[0], "cbz", n, "end");
            arm64_statement(cg, s->children[1]);
        }
        fprintf(cg->output, "L%d_end:\n", n);
        return;
    case AST_WHILE_STATEMENT:
        n = cg->label_count++;
        fprintf(cg->output, "L%d_loop_cond:\n", n);
        arm64_cond_branch(cg, s->children[0], "cbz", n, "loop_end");
        arm64_loop_body(cg, s->children[1], n);
        fprintf(cg->output, "\tb\tL%d_loop_cond\n", n);
        fprintf(cg->output, "L%d_loop_end:\n", n);
        return;
    case AST_DO_WHILE_STATEMENT:
        /* children: [0]=body, [1]=condition */
        n = cg->label_count++;
        fprintf(cg->output, "L%d_loop_body:\n", n);
        arm64_loop_body(cg, s->children[0], n);
        fprintf(cg->output, "L%d_loop_cond:\n", n);
        arm64_cond_branch(cg, s->children[1], "cbnz", n, "loop_body");
        fprintf(cg->output, "L%d_loop_end:\n", n);
        return;
    case AST_FOR_STATEMENT:
        /* children: [0]=init, [1]=cond, [2]=update, [3]=body; the first
         * three may be NULL.  An init declaration is scoped to the loop. */
        n = cg->label_count++;
        cg->scope_depth++;
        if (s->children[0]) {
            if (s->children[0]->type == AST_DECLARATION ||
                s->children[0]->type == AST_DECL_LIST)
                arm64_statement(cg, s->children[0]);
            else
                arm64_expr(cg, s->children[0]);
        }
        fprintf(cg->output, "L%d_loop_top:\n", n);
        if (s->children[1])
            arm64_cond_branch(cg, s->children[1], "cbz", n, "loop_end");
        arm64_loop_body(cg, s->children[3], n);
        fprintf(cg->output, "L%d_loop_cond:\n", n);
        if (s->children[2])
            arm64_expr(cg, s->children[2]);
        fprintf(cg->output, "\tb\tL%d_loop_top\n", n);
        fprintf(cg->output, "L%d_loop_end:\n", n);
        arm64_pop_scope(cg);
        return;
    case AST_SWITCH_STATEMENT:
        arm64_switch(cg, s);
        return;
    case AST_CASE_SECTION:
        /* children: [0]=value, [1]=statement (optional) */
        fprintf(cg->output, "L%d_case:\n", arm64_switch_label(cg, s));
        if (s->child_count > 1) arm64_statement(cg, s->children[1]);
        return;
    case AST_DEFAULT_SECTION:
        /* children: [0]=statement (optional) */
        fprintf(cg->output, "L%d_case:\n", arm64_switch_label(cg, s));
        if (s->child_count > 0) arm64_statement(cg, s->children[0]);
        return;
    case AST_BREAK_STATEMENT:
        /* The parser guarantees an enclosing loop or switch. */
        jp = (Arm64Jump *)vec_get(&cg->jumps, cg->jumps.len - 1);
        if (jp->continue_label < 0)
            fprintf(cg->output, "\tb\tL%d_switch_end\n", jp->break_label);
        else
            fprintf(cg->output, "\tb\tL%d_loop_end\n", jp->break_label);
        return;
    case AST_CONTINUE_STATEMENT:
        /* Innermost loop, passing through any enclosing switch. */
        j = cg->jumps.len;
        while (j > 0) {
            j--;
            jp = (Arm64Jump *)vec_get(&cg->jumps, j);
            if (jp->continue_label >= 0) {
                fprintf(cg->output, "\tb\tL%d_loop_cond\n",
                        jp->continue_label);
                return;
            }
        }
        arm64_unsupported(s, arm64_construct_name(s->type));
        return;
    case AST_LABEL_STATEMENT:
        fprintf(cg->output, "L%s_usr_%s:\n", cg->func_name, s->value);
        if (s->child_count > 0) arm64_statement(cg, s->children[0]);
        return;
    case AST_GOTO_STATEMENT:
        if (!arm64_user_label_defined(cg, s->value))
            compile_error("error: undefined label '%s' in function '%s'\n",
                          s->value, cg->func_name);
        fprintf(cg->output, "\tb\tL%s_usr_%s\n", cg->func_name, s->value);
        return;
    case AST_BLOCK:
        cg->scope_depth++;
        for (i = 0; i < s->child_count; i++)
            arm64_statement(cg, s->children[i]);
        arm64_pop_scope(cg);
        return;
    case AST_DECLARATION:
        arm64_declaration(cg, s);
        return;
    case AST_DECL_LIST:
        for (i = 0; i < s->child_count; i++)
            arm64_declaration(cg, s->children[i]);
        return;
    case AST_EXPRESSION_STMT:
        if (s->child_count > 0) arm64_expr(cg, s->children[0]);
        return;
    case AST_RETURN_STATEMENT:
        if (s->child_count != 1)
            arm64_unsupported(s, "return without a value");
        arm64_expr_as(cg, s->children[0], cg->ret_is_long);
        fprintf(cg->output, "\tb\tL%s_epilogue\n", cg->func_name);
        return;
    default:
        arm64_unsupported(s, arm64_construct_name(s->type));
    }
}

/* ------------------------------------------------------------------ */
/* Functions                                                           */
/* ------------------------------------------------------------------ */

static ASTNode *arm64_function_body(ASTNode *func) {
    int i;
    for (i = func->child_count - 1; i >= 0; i--) {
        if (func->children[i]->type == AST_BLOCK) return func->children[i];
    }
    return NULL;
}

static void arm64_function(CodeGenArm64 *cg, ASTNode *func, ASTNode *body) {
    int i;
    int frame = 0;

    /* Parameters are not supported yet (an empty `()`/`(void)` list
     * produces no AST_PARAM children). */
    for (i = 0; i < func->child_count; i++) {
        if (func->children[i]->type == AST_PARAM)
            arm64_unsupported(func->children[i], "function parameter");
    }
    if (func->decl_type != TYPE_INT && !arm64_kind_is_long(func->decl_type))
        arm64_unsupported_type(func, func->decl_type, 0, " return type");

    /* Pre-scan: one slot per declaration, frame rounded up to 16. */
    arm64_prescan(body, &frame);
    frame = (frame + 15) / 16 * 16;
    if (frame > 4095) {
        arm64_mark(func);
        compile_error("error: arm64-macos: stack frame larger than 4095 "
                      "bytes not yet supported\n");
    }

    vec_clear(&cg->locals);
    cg->scope_depth = 0;
    cg->next_offset = 0;
    cg->frame_size = frame;
    cg->func_name = func->value;
    cg->ret_is_long = arm64_kind_is_long(func->decl_type);
    vec_clear(&cg->jumps);
    vec_clear(&cg->user_labels);
    arm64_collect_user_labels(cg, body);

    if (func->storage_class != SC_STATIC)
        fprintf(cg->output, "\t.globl\t_%s\n", func->value);
    fprintf(cg->output, "\t.p2align\t2\n");
    fprintf(cg->output, "_%s:\n", func->value);
    fprintf(cg->output, "\tstp\tx29, x30, [sp, #-16]!\n");
    fprintf(cg->output, "\tmov\tx29, sp\n");
    if (frame > 0)
        fprintf(cg->output, "\tsub\tsp, sp, #%d\n", frame);

    arm64_statement(cg, body);

    /* Falling off the end: main returns 0 (C99 5.1.2.2.3); other
     * functions get the same harmless value. */
    fprintf(cg->output, "\tmov\tw0, #0\n");
    fprintf(cg->output, "L%s_epilogue:\n", func->value);
    fprintf(cg->output, "\tmov\tsp, x29\n");
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
