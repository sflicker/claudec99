#ifndef CCOMPILER_PARSER_H
#define CCOMPILER_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"
#include "type.h"
#include "vec.h"
#include <stdio.h>

typedef struct {
    char name[MAX_NAME_LEN];
    long value;
} EnumConst;

typedef struct {
    char tag[MAX_NAME_LEN];
    int  is_defined;
} EnumTag;

/* Stage 30: one entry per named struct tag in the translation unit. */
typedef struct {
    char         tag[MAX_NAME_LEN];
    struct Type *type;   /* NULL until the struct body is parsed */
} StructTag;

/* Stage 72: one entry per named union tag in the translation unit. */
typedef struct {
    char         tag[MAX_NAME_LEN];
    struct Type *type;   /* NULL until the union body is parsed */
} UnionTag;

/* Stage 28-01/28-02: entry in the parser-level typedef table. */
typedef struct {
    char         name[MAX_NAME_LEN];
    TypeKind     kind;       /* outermost kind; TYPE_POINTER for pointer typedefs */
    struct Type *full_type;  /* NULL for scalar typedefs; pointer chain for pointer typedefs */
    int          scope_depth; /* 0 = file scope, 1+ = block scope */
} TypedefEntry;

typedef struct {
    char name[MAX_NAME_LEN];
    int param_count;
    int has_definition;
    TypeKind return_type;
    TypeKind param_types[FUNC_MAX_PARAMS];
    /* Stage 23: linkage of the first declaration. SC_NONE and SC_EXTERN
     * are both external; SC_STATIC is internal. */
    StorageClass storage_class;
    /* Stage 57-03: set when the parameter list ends with `...`. */
    int is_variadic;
} FuncSig;

/* Stage 22-02: tracks each file-scope object declaration so the parser
 * can detect duplicates and function/object name conflicts.
 * Stage 23: storage_class tracks extern/static/none linkage.
 * Stage 25-01: full_type carries the complete type chain for function-pointer
 * globals so compatibility between successive declarations can be verified. */
typedef struct {
    char name[MAX_NAME_LEN];
    TypeKind kind;
    StorageClass storage_class;
    struct Type *full_type;
} GlobalObjSig;

typedef struct {
    Lexer *lexer;
    Token current;
    FuncSig funcs[PARSER_MAX_FUNCTIONS];
    int func_count;
    /* Stage 22-02: file-scope object table. */
    GlobalObjSig globals[PARSER_MAX_GLOBALS];
    int global_count;
    int loop_depth;
    int switch_depth;
    /* Stage 28-01: typedef name table with scope tracking. */
    TypedefEntry typedefs[PARSER_MAX_TYPEDEFS];
    int typedef_count;
    int scope_depth; /* 0 = file scope, incremented by each parse_block */
    /* Stage 29: enum constant and tag tables (flat, global scope). */
    EnumConst enum_consts[PARSER_MAX_ENUM_CONSTS];
    int enum_const_count;
    Vec enum_tags;  /* EnumTag; dynamic — stage 95-04 */
    /* Stage 30: struct tag table (flat, global scope). */
    StructTag struct_tags[PARSER_MAX_STRUCT_TAGS];
    int struct_tag_count;
    /* Stage 72: union tag table (flat, global scope). */
    Vec union_tags;  /* UnionTag; dynamic — stage 95-04 */
    /* Stage 75-03: set while parsing the body of a variadic function
     * definition so __builtin_va_start can verify its context. */
    int current_func_is_variadic;
} Parser;

void parser_init(Parser *parser, Lexer *lexer);
ASTNode *parse_translation_unit(Parser *parser);

#endif
