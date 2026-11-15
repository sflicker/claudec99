#ifndef CCOMPILER_CODEGEN_ARM64_H
#define CCOMPILER_CODEGEN_ARM64_H

#include <stdio.h>
#include "ast.h"
#include "vec.h"

/* Stage 181: ARM64 (AArch64) Apple/GNU-syntax code generator for the
 * arm64-macos target.  Phase 1 skeleton — see docs/outlines/
 * arm64-macos-backend-roadmap.md for the phased plan.
 *
 * Stage 182 (Phase 2): signed int/long locals and straight-line integer
 * arithmetic.  Locals live in frame slots addressed from x29. */

/* Stage 182: one block-scope local variable. */
typedef struct {
    const char *name;
    int offset;        /* slot is at [x29, #-offset] */
    int is_long;       /* 1 = 64-bit (long / long long), 0 = 32-bit int */
    int scope_depth;   /* block nesting depth that declared it */
} Arm64Local;

typedef struct {
    FILE *output;
    /* Stage 182: per-function state. */
    Vec locals;              /* Arm64Local, innermost scope last */
    int scope_depth;         /* current block nesting depth */
    int next_offset;         /* bytes of frame handed out so far */
    int frame_size;          /* local area size, multiple of 16 */
    const char *func_name;   /* current function (for the epilogue label) */
    int ret_is_long;         /* current function returns a 64-bit value */
} CodeGenArm64;

void codegen_arm64_init(CodeGenArm64 *cg, FILE *output);
void codegen_arm64_translation_unit(CodeGenArm64 *cg, ASTNode *tu);
void codegen_arm64_free(CodeGenArm64 *cg);

#endif
