#ifndef CCOMPILER_CONSTANTS_H
#define CCOMPILER_CONSTANTS_H

/* All limits below are wrapped in #ifndef guards so they can be cleanly
 * overridden from the command line with -D (e.g. -DFUNC_MAX_PARAMS=32)
 * without triggering a "macro redefined" diagnostic.
 *
 * Many former fixed-capacity limits no longer appear here: as of the 95-xx
 * cleanup stages the parser and code-generator tables that they bounded were
 * converted to dynamic `Vec`/`StrBuf` storage, and the name/tag/label buffers
 * they sized were converted to `const char *` pointers into lexer-owned
 * storage. The macros below are the ones still in effect. */

/* ---- AST limits ---- */

/* Maximum number of child nodes an ASTNode can hold. */
#ifndef AST_MAX_CHILDREN
#define AST_MAX_CHILDREN 64
#endif

/* ---- Type system limits ---- */

/* Maximum number of parameters tracked on a TYPE_FUNCTION node. */
#ifndef FUNC_TYPE_MAX_PARAMS
#define FUNC_TYPE_MAX_PARAMS 16
#endif

/* ---- Parser limits ---- */

/* Maximum number of parameters in a function declaration or definition. */
#ifndef FUNC_MAX_PARAMS
#define FUNC_MAX_PARAMS 16
#endif

/* Maximum ArgSlot entries in a CallLayout (FUNC_MAX_PARAMS + 8 extra for
 * hidden sret slot and variadic overhead). Must be a single integer literal
 * so it can be used as an array dimension in our own compiler. */
#ifndef MAX_CALL_LAYOUT_ITEMS
#define MAX_CALL_LAYOUT_ITEMS 24
#endif

/* ---- Preprocessor limits ---- */

#ifndef MAX_INCLUDE_DEPTH
#define MAX_INCLUDE_DEPTH 64
#endif
#ifndef MAX_COND_DEPTH
#define MAX_COND_DEPTH    64
#endif

#endif
