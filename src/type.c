#include <stddef.h>
#include "type.h"

/*
 * Stage 11-01: these helpers exist so later stages can build the
 * rest of the type system on top of them. Codegen does not call
 * any of them yet.
 */

static Type type_char_singleton  = { TYPE_CHAR,  1, 1, 1 };
static Type type_short_singleton = { TYPE_SHORT, 2, 2, 1 };
static Type type_int_singleton   = { TYPE_INT,   4, 4, 1 };
static Type type_long_singleton  = { TYPE_LONG,  8, 8, 1 };

Type *type_char(void)  { return &type_char_singleton;  }
Type *type_short(void) { return &type_short_singleton; }
Type *type_int(void)   { return &type_int_singleton;   }
Type *type_long(void)  { return &type_long_singleton;  }

const char *type_kind_name(TypeKind kind) {
    switch (kind) {
    case TYPE_CHAR:  return "char";
    case TYPE_SHORT: return "short";
    case TYPE_INT:   return "int";
    case TYPE_LONG:  return "long";
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
    }
    return 0;
}
