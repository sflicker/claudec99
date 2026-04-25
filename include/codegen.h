#ifndef CCOMPILER_CODEGEN_H
#define CCOMPILER_CODEGEN_H

#include <stdio.h>
#include "ast.h"
#include "type.h"

#define MAX_LOCALS 64
#define MAX_BREAK_DEPTH 32
#define MAX_SWITCH_DEPTH 16
#define MAX_SWITCH_LABELS 64
#define MAX_USER_LABELS 64

typedef struct {
    char name[256];
    int offset;
    int size;
    /* Stage 12-02: declared kind so codegen can distinguish a pointer
     * local from a long. `full_type` is set only when kind ==
     * TYPE_POINTER and carries the pointer chain head — needed by
     * dereference codegen to pick the load width and by address-of
     * to construct a `pointer-to-T` result type. */
    TypeKind kind;
    Type *full_type;
} LocalVar;

/* One entry per breakable construct (loop or switch). Switches set
 * continue_label to -1 since `continue` is not valid inside a switch
 * alone; AST_CONTINUE walks down to the nearest loop entry. */
typedef struct {
    int break_label;
    int continue_label;
} BreakFrame;

/* Per-switch context. Populated by a pre-walk of the switch body
 * that collects every case/default node pointer (stopping at nested
 * switches) and assigns each a unique label id. During body emission
 * a case/default node is matched against this table to emit its
 * pre-assigned label. */
typedef struct ASTNode ASTNode;
typedef struct {
    ASTNode *nodes[MAX_SWITCH_LABELS];
    int labels[MAX_SWITCH_LABELS];
    int count;
    int default_label;
} SwitchCtx;

typedef struct {
    FILE *output;
    int label_count;
    LocalVar locals[MAX_LOCALS];
    int local_count;
    int stack_offset;
    int scope_start;
    int push_depth;
    int has_frame;
    BreakFrame break_stack[MAX_BREAK_DEPTH];
    int break_depth;
    SwitchCtx switch_stack[MAX_SWITCH_DEPTH];
    int switch_depth;
    /* Per-function user label table (populated by a pre-walk before
     * body emission; used to reject duplicates and missing goto
     * targets). Assembly names are prefixed by `current_func` so
     * reused label names in different functions never collide. */
    char user_labels[MAX_USER_LABELS][256];
    int user_label_count;
    const char *current_func;
    /* Declared return type of the function currently being emitted —
     * used by AST_RETURN_STATEMENT to convert the return expression
     * to the function's declared return type. */
    TypeKind current_return_type;
    /* Root of the translation unit being emitted; used to look up a
     * callee's AST_FUNCTION_DECL (and through it, the declared
     * parameter types) at each call site for argument conversion. */
    ASTNode *tu_root;
} CodeGen;

void codegen_init(CodeGen *cg, FILE *output);
void codegen_translation_unit(CodeGen *cg, ASTNode *node);

#endif
