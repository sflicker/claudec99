#ifndef CCOMPILER_TYPE_H
#define CCOMPILER_TYPE_H

#include "constants.h"

/*
 * Minimal type system — stage 11-01.
 *
 * Currently used only to annotate local variable declarations with
 * one of four integer types. Codegen still treats every declared
 * variable as a 32-bit int, so most helpers below are stubs that
 * later stages will flesh out.
 */

typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,
    TYPE_LONG_LONG,
    TYPE_UNSIGNED_LONG_LONG,
    TYPE_POINTER,
    TYPE_ARRAY,
    /* Stage 25-01: function type — used only as the base of a function-pointer
     * TYPE_POINTER node. `base` = return type, `param_count` = number of
     * parameters, `params[]` = parameter types. Never allocated directly as a
     * variable; always wrapped in at least one TYPE_POINTER. */
    TYPE_FUNCTION,
    /* Stage 30: named struct type. size and alignment are computed from the
     * field layout at definition time. */
    TYPE_STRUCT,
    /* Stage 72: named union type. Layout: all members at offset 0,
     * size = max member size rounded up to alignment. */
    TYPE_UNION
} TypeKind;

/* Stage 31: field descriptor stored inside a TYPE_STRUCT Type node. */
typedef struct {
    char name[MAX_NAME_LEN];
    int  offset;            /* byte offset of this field within the struct */
    TypeKind kind;
    struct Type *full_type; /* non-NULL for pointer/array/struct fields */
    int  is_const;          /* Stage 82-01: set for const-qualified scalar members */
} StructField;

typedef struct Type {
    TypeKind kind;
    int size;
    int alignment;
    int is_signed;
    struct Type *base;
    /* Stage 13-01: number of elements when kind == TYPE_ARRAY. The
     * total byte count lives in `size`; `base` carries the element
     * type. Unused for non-array kinds. */
    int length;
    /* Stage 25-01: parameter count and types for TYPE_FUNCTION nodes. */
    int param_count;
    struct Type *params[FUNC_TYPE_MAX_PARAMS];
    /* Stage 31: field table for TYPE_STRUCT nodes. NULL for other kinds.
     * Points to a calloc'd array of field_count StructField entries. */
    StructField *fields;
    int field_count;
    /* Stage 66: set when this type is const-qualified. Used on
     * heap-allocated copies only — never set on static singletons. */
    int is_const;
} Type;

Type *type_void(void);
Type *type_bool(void);
Type *type_char(void);
Type *type_short(void);
Type *type_int(void);
Type *type_long(void);
Type *type_long_long(void);
Type *type_unsigned_long_long(void);
Type *type_unsigned_char(void);
Type *type_unsigned_short(void);
Type *type_unsigned_int(void);
Type *type_unsigned_long(void);
Type *type_pointer(Type *base);
Type *type_array(Type *element_type, int length);
Type *type_function(Type *return_type, int param_count, Type **param_types);
Type *type_struct(int size, int alignment);
Type *type_union(int size, int alignment);
/* Stage 66: allocate a heap copy of t with is_const=1. Never mutates t. */
Type *type_const_copy(Type *t);

const char *type_kind_name(TypeKind kind);
int type_size(Type *t);
int type_alignment(Type *t);
int type_is_integer(Type *t);

#endif
