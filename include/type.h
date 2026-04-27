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

typedef enum {
    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,
    TYPE_POINTER,
    TYPE_ARRAY
} TypeKind;

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
} Type;

Type *type_char(void);
Type *type_short(void);
Type *type_int(void);
Type *type_long(void);
Type *type_pointer(Type *base);
Type *type_array(Type *element_type, int length);

const char *type_kind_name(TypeKind kind);
int type_size(Type *t);
int type_alignment(Type *t);
int type_is_integer(Type *t);

#endif
