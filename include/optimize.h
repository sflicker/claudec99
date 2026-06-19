#ifndef CCOMPILER_OPTIMIZE_H
#define CCOMPILER_OPTIMIZE_H

#include "ast.h"

/*
 * optimize_translation_unit — AST-level optimization pass.
 *
 * Walks every function body in the translation unit and rewrites constant
 * expressions, dead branches, and algebraic identities.  The returned
 * pointer is the (possibly replaced) root; callers must use the return
 * value rather than the original pointer.
 *
 * opt_level: 0 = disabled (no-op, returns root unchanged).
 *            1 = AST-level constant folding and dead-code elimination.
 */
ASTNode *optimize_translation_unit(ASTNode *root, int opt_level);

#endif
