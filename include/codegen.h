#ifndef CCOMPILER_CODEGEN_H
#define CCOMPILER_CODEGEN_H

#include <stdio.h>
#include "ast.h"

#define MAX_LOCALS 64
#define MAX_BREAK_DEPTH 32
#define MAX_SWITCH_DEPTH 16
#define MAX_SWITCH_LABELS 64

typedef struct {
    char name[256];
    int offset;
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
} CodeGen;

void codegen_init(CodeGen *cg, FILE *output);
void codegen_translation_unit(CodeGen *cg, ASTNode *node);

#endif
