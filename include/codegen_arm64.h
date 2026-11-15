#ifndef CCOMPILER_CODEGEN_ARM64_H
#define CCOMPILER_CODEGEN_ARM64_H

#include <stdio.h>
#include "ast.h"

/* Stage 181: ARM64 (AArch64) Apple/GNU-syntax code generator for the
 * arm64-macos target.  Phase 1 skeleton — see docs/outlines/
 * arm64-macos-backend-roadmap.md for the phased plan. */
typedef struct {
    FILE *output;
} CodeGenArm64;

void codegen_arm64_init(CodeGenArm64 *cg, FILE *output);
void codegen_arm64_translation_unit(CodeGenArm64 *cg, ASTNode *tu);
void codegen_arm64_free(CodeGenArm64 *cg);

#endif
