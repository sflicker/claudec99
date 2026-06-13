#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "type.h"

/*
 * Stage 11-01: these helpers exist so later stages can build the
 * rest of the type system on top of them. Codegen does not call
 * any of them yet.
 */

static Type type_void_singleton                = { TYPE_VOID,                 0, 0, 0, NULL, 0 };
static Type type_bool_singleton                = { TYPE_BOOL,                 1, 1, 0, NULL, 0 };
static Type type_char_singleton                = { TYPE_CHAR,                 1, 1, 1, NULL, 0 };
static Type type_short_singleton               = { TYPE_SHORT,                2, 2, 1, NULL, 0 };
static Type type_int_singleton                 = { TYPE_INT,                  4, 4, 1, NULL, 0 };
static Type type_long_singleton                = { TYPE_LONG,                 8, 8, 1, NULL, 0 };
static Type type_long_long_singleton           = { TYPE_LONG_LONG,            8, 8, 1, NULL, 0 };
static Type type_unsigned_char_singleton       = { TYPE_CHAR,                 1, 1, 0, NULL, 0 };
static Type type_unsigned_short_singleton      = { TYPE_SHORT,                2, 2, 0, NULL, 0 };
static Type type_unsigned_int_singleton        = { TYPE_INT,                  4, 4, 0, NULL, 0 };
static Type type_unsigned_long_singleton       = { TYPE_LONG,                 8, 8, 0, NULL, 0 };
static Type type_unsigned_long_long_singleton  = { TYPE_UNSIGNED_LONG_LONG,   8, 8, 0, NULL, 0 };
static Type type_float_singleton               = { TYPE_FLOAT,                 4, 4, 0, NULL, 0 };
static Type type_double_singleton              = { TYPE_DOUBLE,                8, 8, 0, NULL, 0 };

Type *type_void(void)                { return &type_void_singleton;                }
Type *type_bool(void)                { return &type_bool_singleton;                }
Type *type_char(void)                { return &type_char_singleton;                }
Type *type_short(void)               { return &type_short_singleton;               }
Type *type_int(void)                 { return &type_int_singleton;                 }
Type *type_long(void)                { return &type_long_singleton;                }
Type *type_long_long(void)           { return &type_long_long_singleton;           }
Type *type_unsigned_char(void)       { return &type_unsigned_char_singleton;       }
Type *type_unsigned_short(void)      { return &type_unsigned_short_singleton;      }
Type *type_unsigned_int(void)        { return &type_unsigned_int_singleton;        }
Type *type_unsigned_long(void)       { return &type_unsigned_long_singleton;       }
Type *type_unsigned_long_long(void)  { return &type_unsigned_long_long_singleton;  }
Type *type_float(void)               { return &type_float_singleton;               }
Type *type_double(void)              { return &type_double_singleton;              }

/*
 * Stage 12-01: heap-allocate a pointer Type that wraps `base`.
 * Pointer chains can nest arbitrarily, so a static singleton
 * approach is not used here. The compiler does not free Types.
 */
Type *type_pointer(Type *base) {
    Type *t = calloc(1, sizeof(Type));
    if (!t) {
        fprintf(stderr, "error: out of memory\n");
        exit(1);
    }
    t->kind = TYPE_POINTER;
    t->size = 8;
    t->alignment = 8;
    t->is_signed = 0;
    t->base = base;
    return t;
}

/*
 * Stage 13-01: heap-allocate an array Type. `size` records the total
 * byte count (element_size * length); `alignment` mirrors the
 * element's natural alignment. `base` carries the element type.
 */
Type *type_array(Type *element_type, int length) {
    Type *t = calloc(1, sizeof(Type));
    if (!t) {
        fprintf(stderr, "error: out of memory\n");
        exit(1);
    }
    t->kind = TYPE_ARRAY;
    t->size = element_type->size * length;
    t->alignment = element_type->alignment;
    t->is_signed = 0;
    t->base = element_type;
    t->length = length;
    return t;
}

/*
 * Stage 25-01: heap-allocate a TYPE_FUNCTION node. `base` = return type;
 * `param_count` and `params[]` carry the parameter types. Size and
 * alignment are 0 because a function type is never directly allocated;
 * it always appears as the base of a TYPE_POINTER (the function pointer).
 */
Type *type_function(Type *return_type, int param_count, Type **param_types) {
    Type *t = calloc(1, sizeof(Type));
    if (!t) {
        fprintf(stderr, "error: out of memory\n");
        exit(1);
    }
    t->kind = TYPE_FUNCTION;
    t->size = 0;
    t->alignment = 0;
    t->is_signed = 0;
    t->base = return_type;
    t->param_count = param_count;
    for (int i = 0; i < param_count && i < FUNC_TYPE_MAX_PARAMS; i++)
        t->params[i] = param_types[i];
    return t;
}

/* Stage 66: heap-allocate a const-qualified copy of t. Never mutates
 * the original — callers must not modify the returned node's base fields
 * (kind, size, alignment, is_signed, base) as those are shallow-copied. */
Type *type_const_copy(Type *t) {
    Type *c = calloc(1, sizeof(Type));
    if (!c) {
        fprintf(stderr, "error: out of memory\n");
        exit(1);
    }
    *c = *t;
    c->is_const = 1;
    return c;
}

/* Stage 82-04: heap-allocate a volatile-qualified copy of t. Never mutates
 * the original — callers must not modify the returned node's base fields. */
Type *type_volatile_copy(Type *t) {
    Type *v = calloc(1, sizeof(Type));
    if (!v) {
        fprintf(stderr, "error: out of memory\n");
        exit(1);
    }
    *v = *t;
    v->is_volatile = 1;
    return v;
}

/* Stage 30: heap-allocate a TYPE_STRUCT node with precomputed size and
 * alignment. Fields are not stored in the Type — only the layout totals
 * needed for sizeof and stack allocation. */
Type *type_struct(int size, int alignment) {
    Type *t = calloc(1, sizeof(Type));
    if (!t) {
        fprintf(stderr, "error: out of memory\n");
        exit(1);
    }
    t->kind = TYPE_STRUCT;
    t->size = size;
    t->alignment = alignment;
    t->is_signed = 0;
    return t;
}

/* Stage 72: heap-allocate a TYPE_UNION node. Layout mirrors type_struct
 * except the kind is TYPE_UNION. */
Type *type_union(int size, int alignment) {
    Type *t = calloc(1, sizeof(Type));
    if (!t) {
        fprintf(stderr, "error: out of memory\n");
        exit(1);
    }
    t->kind = TYPE_UNION;
    t->size = size;
    t->alignment = alignment;
    t->is_signed = 0;
    return t;
}

const char *type_kind_name(TypeKind kind) {
    switch (kind) {
    case TYPE_VOID:     return "void";
    case TYPE_BOOL:     return "_Bool";
    case TYPE_CHAR:     return "char";
    case TYPE_SHORT:    return "short";
    case TYPE_INT:      return "int";
    case TYPE_LONG:               return "long";
    case TYPE_LONG_LONG:          return "long long";
    case TYPE_UNSIGNED_LONG_LONG: return "unsigned long long";
    case TYPE_FLOAT:              return "float";
    case TYPE_DOUBLE:             return "double";
    case TYPE_POINTER:            return "pointer";
    case TYPE_ARRAY:    return "array";
    case TYPE_FUNCTION: return "function";
    case TYPE_STRUCT:   return "struct";
    case TYPE_UNION:    return "union";
    }
    return "unknown";
}

int type_size(Type *t) {
    return t ? t->size : 0;
}

int type_alignment(Type *t) {
    return t ? t->alignment : 0;
}

int type_is_integer(Type *t) {
    if (!t) return 0;
    switch (t->kind) {
    case TYPE_BOOL:
    case TYPE_CHAR:
    case TYPE_SHORT:
    case TYPE_INT:
    case TYPE_LONG:
    case TYPE_LONG_LONG:
    case TYPE_UNSIGNED_LONG_LONG:
        return 1;
    case TYPE_FLOAT:
    case TYPE_DOUBLE:
    case TYPE_VOID:
    case TYPE_POINTER:
    case TYPE_ARRAY:
    case TYPE_FUNCTION:
    case TYPE_STRUCT:
    case TYPE_UNION:
        return 0;
    }
    return 0;
}
