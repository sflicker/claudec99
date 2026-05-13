#ifndef CCOMPILER_TYPE_H
#define CCOMPILER_TYPE_H

/*
 * Minimal type system — stage 11-01.
 *
 * Currently used only to annotate local variable declarations with
 * one of four integer types. Codegen still treats every declared
 * variable as a 32-bit int, so most helpers below are stubs that
 * later stages will flesh out.
 */

/* Stage 25-01: maximum number of parameters tracked on a TYPE_FUNCTION node. */
#define FUNC_TYPE_MAX_PARAMS 16

typedef enum {
    TYPE_VOID,
    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,
    TYPE_POINTER,
    TYPE_ARRAY,
    /* Stage 25-01: function type — used only as the base of a function-pointer
     * TYPE_POINTER node. `base` = return type, `param_count` = number of
     * parameters, `params[]` = parameter types. Never allocated directly as a
     * variable; always wrapped in at least one TYPE_POINTER. */
    TYPE_FUNCTION,
    /* Stage 30: named struct type. size and alignment are computed from the
     * field layout at definition time. */
    TYPE_STRUCT
} TypeKind;

/* Stage 31: field descriptor stored inside a TYPE_STRUCT Type node. */
typedef struct {
    char name[256];
    int  offset;            /* byte offset of this field within the struct */
    TypeKind kind;
    struct Type *full_type; /* non-NULL for pointer/array/struct fields */
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
} Type;

Type *type_void(void);
Type *type_char(void);
Type *type_short(void);
Type *type_int(void);
Type *type_long(void);
Type *type_pointer(Type *base);
Type *type_array(Type *element_type, int length);
Type *type_function(Type *return_type, int param_count, Type **param_types);
Type *type_struct(int size, int alignment);

const char *type_kind_name(TypeKind kind);
int type_size(Type *t);
int type_alignment(Type *t);
int type_is_integer(Type *t);

#endif
