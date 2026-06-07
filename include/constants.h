#ifndef CCOMPILER_CONSTANTS_H
#define CCOMPILER_CONSTANTS_H

/* All limits below are wrapped in #ifndef guards so they can be cleanly
 * overridden from the command line with -D (e.g. -DPARSER_MAX_FUNCTIONS=256)
 * without triggering a "macro redefined" diagnostic. */

/* Maximum length of any identifier, tag name, assembly label, or value
 * string stored in a fixed-size character array throughout the compiler. */
#ifndef MAX_NAME_LEN
#define MAX_NAME_LEN 256
#endif

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

#ifndef PARSER_MAX_GLOBALS
#define PARSER_MAX_GLOBALS       256
#endif
#ifndef PARSER_MAX_ENUM_CONSTS
#define PARSER_MAX_ENUM_CONSTS   256
#endif
#ifndef PARSER_MAX_ENUM_TAGS
#define PARSER_MAX_ENUM_TAGS     32
#endif
#ifndef PARSER_MAX_STRUCT_TAGS
#define PARSER_MAX_STRUCT_TAGS   32
#endif
#ifndef PARSER_MAX_UNION_TAGS
#define PARSER_MAX_UNION_TAGS    32
#endif
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

/* ---- Codegen limits ---- */

#ifndef MAX_LOCALS
#define MAX_LOCALS          256
#endif
#ifndef MAX_GLOBALS
#define MAX_GLOBALS         256
#endif
#ifndef MAX_SWITCH_DEPTH
#define MAX_SWITCH_DEPTH    16
#endif
#ifndef MAX_SWITCH_LABELS
/* Stage 92: raised from 64 so the compiler can self-compile. token_type_name()
 * in compiler.c switches over ~83 token kinds in a single switch. */
#define MAX_SWITCH_LABELS   256
#endif
#ifndef MAX_USER_LABELS
#define MAX_USER_LABELS     64
#endif
#ifndef MAX_STRING_LITERALS
/* Stage 92: raised from 256 so the compiler can self-compile. codegen.c alone
 * uses ~750 string-literal occurrences (the pool does not deduplicate). */
#define MAX_STRING_LITERALS 2048
#endif
#ifndef MAX_LOCAL_STATICS
#define MAX_LOCAL_STATICS   128
#endif

/* ---- Preprocessor limits ---- */

#ifndef MAX_INCLUDE_DEPTH
#define MAX_INCLUDE_DEPTH 64
#endif
#ifndef MAX_COND_DEPTH
#define MAX_COND_DEPTH    64
#endif

#endif
