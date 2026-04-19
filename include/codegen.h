#ifndef CCOMPILER_CODEGEN_H
#define CCOMPILER_CODEGEN_H

#include <stdio.h>
#include "ast.h"

#define MAX_LOCALS 64
#define MAX_LOOP_DEPTH 32

typedef struct {
    char name[256];
    int offset;
} LocalVar;

typedef struct {
    int break_label;
    int continue_label;
} LoopLabels;

typedef struct {
    FILE *output;
    int label_count;
    LocalVar locals[MAX_LOCALS];
    int local_count;
    int stack_offset;
    int scope_start;
    int push_depth;
    int has_frame;
    LoopLabels loop_stack[MAX_LOOP_DEPTH];
    int loop_depth;
} CodeGen;

void codegen_init(CodeGen *cg, FILE *output);
void codegen_translation_unit(CodeGen *cg, ASTNode *node);

#endif
