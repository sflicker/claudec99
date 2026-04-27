#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "type.h"

/*
 * Stage 11-01: these helpers exist so later stages can build the
 * rest of the type system on top of them. Codegen does not call
 * any of them yet.
 */

static Type type_char_singleton  = { TYPE_CHAR,  1, 1, 1, NULL, 0 };
static Type type_short_singleton = { TYPE_SHORT, 2, 2, 1, NULL, 0 };
static Type type_int_singleton   = { TYPE_INT,   4, 4, 1, NULL, 0 };
static Type type_long_singleton  = { TYPE_LONG,  8, 8, 1, NULL, 0 };

Type *type_char(void)  { return &type_char_singleton;  }
Type *type_short(void) { return &type_short_singleton; }
Type *type_int(void)   { return &type_int_singleton;   }
Type *type_long(void)  { return &type_long_singleton;  }

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

const char *type_kind_name(TypeKind kind) {
    switch (kind) {
    case TYPE_CHAR:    return "char";
    case TYPE_SHORT:   return "short";
    case TYPE_INT:     return "int";
    case TYPE_LONG:    return "long";
    case TYPE_POINTER: return "pointer";
    case TYPE_ARRAY:   return "array";
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
    case TYPE_CHAR:
    case TYPE_SHORT:
    case TYPE_INT:
    case TYPE_LONG:
        return 1;
    case TYPE_POINTER:
    case TYPE_ARRAY:
        return 0;
    }
    return 0;
}
