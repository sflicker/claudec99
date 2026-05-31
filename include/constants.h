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

#ifndef PARSER_MAX_FUNCTIONS
#define PARSER_MAX_FUNCTIONS     64
#endif
#ifndef PARSER_MAX_GLOBALS
#define PARSER_MAX_GLOBALS       64
#endif
#ifndef PARSER_MAX_TYPEDEFS
#define PARSER_MAX_TYPEDEFS      64
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
/* Maximum number of fields in a single struct or union definition. */
#ifndef PARSER_MAX_STRUCT_FIELDS
#define PARSER_MAX_STRUCT_FIELDS 64
#endif

/* Maximum number of parameters in a function declaration or definition. */
#ifndef FUNC_MAX_PARAMS
#define FUNC_MAX_PARAMS 16
#endif

/* ---- Codegen limits ---- */

#ifndef MAX_LOCALS
#define MAX_LOCALS          64
#endif
#ifndef MAX_GLOBALS
#define MAX_GLOBALS         64
#endif
#ifndef MAX_BREAK_DEPTH
#define MAX_BREAK_DEPTH     32
#endif
#ifndef MAX_SWITCH_DEPTH
#define MAX_SWITCH_DEPTH    16
#endif
#ifndef MAX_SWITCH_LABELS
#define MAX_SWITCH_LABELS   64
#endif
#ifndef MAX_USER_LABELS
#define MAX_USER_LABELS     64
#endif
#ifndef MAX_STRING_LITERALS
#define MAX_STRING_LITERALS 256
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
