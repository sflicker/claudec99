#ifndef CCOMPILER_CONSTANTS_H
#define CCOMPILER_CONSTANTS_H

/* Maximum length of any identifier, tag name, assembly label, or value
 * string stored in a fixed-size character array throughout the compiler. */
#define MAX_NAME_LEN 256

/* ---- AST limits ---- */

/* Maximum number of child nodes an ASTNode can hold. */
#define AST_MAX_CHILDREN 64

/* ---- Type system limits ---- */

/* Maximum number of parameters tracked on a TYPE_FUNCTION node. */
#define FUNC_TYPE_MAX_PARAMS 16

/* ---- Parser limits ---- */

#define PARSER_MAX_FUNCTIONS     64
#define PARSER_MAX_GLOBALS       64
#define PARSER_MAX_TYPEDEFS      64
#define PARSER_MAX_ENUM_CONSTS   256
#define PARSER_MAX_ENUM_TAGS     32
#define PARSER_MAX_STRUCT_TAGS   32
#define PARSER_MAX_UNION_TAGS    32
/* Maximum number of fields in a single struct or union definition. */
#define PARSER_MAX_STRUCT_FIELDS 64

/* Maximum number of parameters in a function declaration or definition. */
#define FUNC_MAX_PARAMS 16

/* ---- Codegen limits ---- */

#define MAX_LOCALS          64
#define MAX_GLOBALS         64
#define MAX_BREAK_DEPTH     32
#define MAX_SWITCH_DEPTH    16
#define MAX_SWITCH_LABELS   64
#define MAX_USER_LABELS     64
#define MAX_STRING_LITERALS 256
#define MAX_LOCAL_STATICS   128

/* ---- Preprocessor limits ---- */

#define MAX_INCLUDE_DEPTH 64
#define MAX_COND_DEPTH    64

#endif
