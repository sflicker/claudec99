#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "strbuf.h"
#include "util.h"

/* Stage 70-03: report a parser error at the current token's source position. */
#define PARSER_ERROR(parser, ...) \
    compile_error_at( \
        (parser)->current.file ? (parser)->current.file->path : NULL, \
        (parser)->current.line, \
        (parser)->current.col, \
        __VA_ARGS__)

/* Stage 86: create an AST node stamped with the current token's source
 * position so codegen and semantic errors can report file:line:col. Every
 * node-creating call in the parser goes through this rather than ast_new
 * directly. parser->current is the token at the point of construction
 * (restored correctly across lookahead/peek), so its position locates the
 * construct closely enough for diagnostics. */
static ASTNode *parser_node(Parser *parser, ASTNodeType type, const char *value) {
    ASTNode *node = ast_new(type, value);
    node->src_file = parser->current.file ? parser->current.file->path : NULL;
    node->src_line = parser->current.line;
    node->src_col  = parser->current.col;
    return node;
}

/*
 * Internal helper filled by parse_declarator. Carries the identifier
 * name, pointer depth, and optional array-size or function-suffix
 * information parsed out of one declarator before the caller assembles
 * the semantic Type.
 *
 * is_function: set when the direct_declarator has the form
 *   <identifier> "(" ... ")"
 * The "(" is NOT consumed here; the caller handles it.
 *
 * Stage 25-01: is_func_pointer is set when the declarator has the form
 *   [outer-stars] "(" inner-stars <identifier> ")" "(" ... ")"
 * e.g. int (*fp)(int).  fp_outer_stars counts the stars before the
 * opening "("; those become part of the function's return type.
 * fp_inner_stars counts the stars inside "(*)"; those become the
 * pointer-to-function depth (normally 1).
 *
 * Stage 26: when is_func_pointer is set, the trailing "(" parameter list
 * ")" is consumed immediately inside parse_declarator. fp_param_types and
 * fp_param_count carry the result; callers build the full Type* inline.
 */
/* Stage 86: maximum number of array dimensions in a single declarator. */
#define MAX_ARRAY_DIMS 8

typedef struct {
    const char *name;
    int  pointer_count;
    int  is_array;
    int  has_size;
    int  array_length;      /* first dimension — kept for backward compatibility */
    /* Stage 86: all dimensions for multi-dimensional array declarators. */
    int  array_dims[MAX_ARRAY_DIMS];
    int  array_dim_count;
    int  is_function;
    int  is_func_pointer;
    int  fp_outer_stars;
    int  fp_inner_stars;
    /* Stage 26: param types consumed inline when is_func_pointer is set. */
    Type *fp_param_types[FUNC_TYPE_MAX_PARAMS];
    int   fp_param_count;
    /* Stage 66: set when "const" appears after the last "*" (T * const p). */
    int  pointer_is_const;
    /* Stage 135: set when the parenthesized form (*name) is followed by [N]
     * or [] — a pointer-to-array declarator (e.g. int (*row)[4]).
     * ptr_to_array_length is the bound N; ptr_to_array_has_size is 1 when N
     * is explicit and 0 for the incomplete [] form. */
    int  is_ptr_to_array;
    int  ptr_to_array_length;
    int  ptr_to_array_has_size;
    /* Stage 137: set when the declarator is (*name())(params) — a function
     * returning a pointer-to-function.  own_param_* hold the outer function's
     * own parameter types (parsed inline); fp_param_* (existing) hold the
     * returned function pointer's parameter list. */
    int  is_func_returning_fp;
    Type *own_param_types[FUNC_TYPE_MAX_PARAMS];
    int   own_param_count;
    int   own_is_no_prototype;
} ParsedDeclarator;

void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->current = lexer_next_token(lexer);
    vec_init(&parser->funcs, sizeof(FuncSig));
    parser->func_count = 0;
    vec_init(&parser->globals, sizeof(GlobalObjSig));
    parser->loop_depth = 0;
    parser->switch_depth = 0;
    vec_init(&parser->typedefs, sizeof(TypedefEntry));
    parser->typedef_count = 0;
    parser->scope_depth = 0;
    vec_init(&parser->enum_consts, sizeof(EnumConst));
    vec_init(&parser->enum_tags, sizeof(EnumTag));
    vec_init(&parser->struct_tags, sizeof(StructTag));
    vec_init(&parser->union_tags, sizeof(UnionTag));
    parser->current_func_is_variadic = 0;
    parser->current_func_name        = NULL;
}

/* Stage 96: release all Vec backing buffers owned by the parser.
 * Element strings point into lexer-owned storage and are not freed. */
void parser_free(Parser *parser) {
    vec_free(&parser->funcs);
    vec_free(&parser->globals);
    vec_free(&parser->typedefs);
    vec_free(&parser->enum_consts);
    vec_free(&parser->enum_tags);
    vec_free(&parser->struct_tags);
    vec_free(&parser->union_tags);
}

/* Stage 28-01: typedef name table helpers. */
static TypedefEntry *parser_find_typedef(Parser *parser, const char *name) {
    for (int i = parser->typedef_count - 1; i >= 0; i--) {
        TypedefEntry *e = (TypedefEntry *)vec_get(&parser->typedefs, (size_t)i);
        if (strcmp(e->name, name) == 0)
            return e;
    }
    return NULL;
}

static void parser_register_typedef(Parser *parser, const char *name,
                                    TypeKind kind, Type *full_type) {
    for (int i = 0; i < parser->typedef_count; i++) {
        TypedefEntry *e = (TypedefEntry *)vec_get(&parser->typedefs, (size_t)i);
        if (e->scope_depth == parser->scope_depth &&
            strcmp(e->name, name) == 0) {
            PARSER_ERROR(parser, "error: duplicate typedef '%s' in this scope\n",
                    name);
        }
    }
    {
        TypedefEntry entry;
        memset(&entry, 0, sizeof(entry));
        entry.name = name;
        entry.kind = kind;
        entry.full_type = full_type;
        entry.scope_depth = parser->scope_depth;
        vec_push(&parser->typedefs, &entry);
    }
    parser->typedef_count++;
}

static void parser_leave_scope(Parser *parser) {
    int new_count = 0;
    for (int i = 0; i < parser->typedef_count; i++) {
        TypedefEntry *e = (TypedefEntry *)vec_get(&parser->typedefs, (size_t)i);
        if (e->scope_depth < parser->scope_depth) {
            if (new_count != i) {
                TypedefEntry *dst = (TypedefEntry *)vec_get(&parser->typedefs,
                                                            (size_t)new_count);
                *dst = *e;
            }
            new_count++;
        }
    }
    parser->typedefs.len = (size_t)new_count;
    parser->typedef_count = new_count;
    parser->scope_depth--;
}

static FuncSig *parser_find_function(Parser *parser, const char *name) {
    for (int i = 0; i < parser->func_count; i++) {
        FuncSig *sig = (FuncSig *)vec_get(&parser->funcs, (size_t)i);
        if (strcmp(sig->name, name) == 0) {
            return sig;
        }
    }
    return NULL;
}

/* Stage 22-02: file-scope object tracking. */
static GlobalObjSig *parser_find_global(Parser *parser, const char *name) {
    for (size_t i = 0; i < parser->globals.len; i++) {
        GlobalObjSig *g = (GlobalObjSig *)vec_get(&parser->globals, i);
        if (strcmp(g->name, name) == 0)
            return g;
    }
    return NULL;
}

/*
 * Stage 25-01: deep equality check for function-pointer types.
 * Both a and b are expected to be TYPE_POINTER → TYPE_FUNCTION chains.
 * Compares return type kind, parameter count, and each parameter's kind.
 * Returns 1 if compatible, 0 if not.
 */
static int func_ptr_types_equal(Type *a, Type *b) {
    if (!a || !b) return a == b;
    if (a->kind != TYPE_POINTER || b->kind != TYPE_POINTER) return 0;
    Type *fa = a->base, *fb = b->base;
    if (!fa || !fb || fa->kind != TYPE_FUNCTION || fb->kind != TYPE_FUNCTION)
        return 0;
    if (!fa->base || !fb->base || fa->base->kind != fb->base->kind) return 0;
    if (fa->param_count != fb->param_count) return 0;
    for (int i = 0; i < fa->param_count; i++) {
        if (!fa->params[i] || !fb->params[i]) return 0;
        if (fa->params[i]->kind != fb->params[i]->kind) return 0;
    }
    return 1;
}

/* Stage 23: register a file-scope object with storage class.
 * Repeated extern declarations are allowed; conflicting linkage
 * (static vs non-static) and duplicate definitions (non-extern + non-extern)
 * are rejected.
 * Stage 25-01: full_type carries the complete type for function-pointer
 * globals so that successive declarations are checked for compatibility. */
/* Stage 126: has_init is 1 if this declaration includes an initializer
 * (i.e., it is a complete definition), 0 if it is a tentative definition. */
static void parser_register_global(Parser *parser, const char *name,
                                   TypeKind kind, StorageClass sc,
                                   Type *full_type, int has_init) {
    GlobalObjSig *existing = parser_find_global(parser, name);
    if (existing) {
        if (existing->kind != kind) {
            PARSER_ERROR(parser, "error: conflicting type for global '%s'\n", name);
        }
        /* Stage 25-01: for function-pointer globals, verify the full type. */
        if (kind == TYPE_POINTER && existing->full_type && full_type) {
            Type *ef = existing->full_type, *nf = full_type;
            if (ef->base && nf->base &&
                ef->base->kind == TYPE_FUNCTION &&
                nf->base->kind == TYPE_FUNCTION) {
                if (!func_ptr_types_equal(ef, nf)) {
                    PARSER_ERROR(parser,
                            "error: conflicting type for global '%s'\n", name);
                }
            }
        }
        int ex_is_static = (existing->storage_class == SC_STATIC);
        int new_is_static = (sc == SC_STATIC);
        if (ex_is_static != new_is_static) {
            PARSER_ERROR(parser, "error: conflicting linkage for '%s'\n", name);
        }
        if (ex_is_static) {
            /* Stage 126: static + static — two definitions are an error;
             * tentative + tentative or tentative + definition are legal. */
            if (existing->is_defined && has_init) {
                PARSER_ERROR(parser, "error: duplicate definition of '%s'\n", name);
            }
            if (has_init) existing->is_defined = 1;
            return;
        }
        /* Both non-static: extern+extern and extern+none are OK.
         * Stage 126: none+none — allow if at most one has an initializer
         * (tentative definition merging per C99 §6.9.2). */
        if (existing->storage_class != SC_EXTERN && sc != SC_EXTERN) {
            if (existing->is_defined && has_init) {
                PARSER_ERROR(parser, "error: duplicate definition of '%s'\n", name);
            }
            if (has_init) existing->is_defined = 1;
            return;
        }
        /* If new declaration is a definition (SC_NONE), upgrade the entry. */
        if (sc == SC_NONE) {
            existing->storage_class = SC_NONE;
        }
        return;
    }
    GlobalObjSig new_g;
    new_g.name = name;
    new_g.kind = kind;
    new_g.storage_class = sc;
    new_g.full_type = full_type;
    new_g.is_defined = has_init;
    vec_push(&parser->globals, &new_g);
}

/*
 * Record a function's appearance (declaration or definition). If the name is
 * already in the table, enforce:
 *   - parameter counts must match
 *   - at most one definition per name
 * Otherwise add a new entry. The full typed signature (return type and
 * ordered parameter types) is captured from the first occurrence — per
 * spec, signature compatibility between declarations and definitions
 * beyond arity is not checked in this stage.
 */
/* Stage 23: sc is the storage class of this declaration/definition.
 * SC_STATIC means internal linkage; SC_NONE and SC_EXTERN both mean
 * external linkage. Mixing static and non-static for the same name
 * is a linkage conflict. */
static void parser_register_function(Parser *parser, const char *name,
                                     int param_count, int is_definition,
                                     TypeKind return_type,
                                     const TypeKind *param_types,
                                     StorageClass sc, int is_variadic,
                                     int is_no_prototype) {
    /* Stage 22-02: reject function if a global object with the same name exists. */
    if (parser_find_global(parser, name)) {
        PARSER_ERROR(parser,
                "error: '%s' redeclared as a different kind of symbol\n", name);
    }
    FuncSig *existing = parser_find_function(parser, name);
    if (existing) {
        /* Stage 23: check for static vs non-static linkage conflict. */
        int ex_is_static = (existing->storage_class == SC_STATIC);
        int new_is_static = (sc == SC_STATIC);
        if (ex_is_static != new_is_static) {
            PARSER_ERROR(parser,
                    "error: conflicting linkage for '%s'\n", name);
        }
        /* Stage 133: no-prototype declaration is compatible with any later
         * prototype definition — update the entry with the actual param count. */
        if (existing->has_no_prototype && param_count >= 0) {
            existing->param_count = param_count;
            existing->return_type = return_type;
            if (param_types) {
                for (int i = 0; i < param_count; i++)
                    existing->param_types[i] = param_types[i];
            }
        /* Stage 129: -1 means "unknown arity" from a block-scope forward decl;
         * allow a real declaration/definition to override it. */
        } else if (existing->param_count == -1) {
            existing->param_count = param_count;
            existing->return_type = return_type;
            if (param_types) {
                for (int i = 0; i < param_count; i++)
                    existing->param_types[i] = param_types[i];
            }
        } else if (param_count != -1 && existing->param_count != param_count) {
            PARSER_ERROR(parser,
                    "error: function '%s' parameter count mismatch (%d vs %d)\n",
                    name, existing->param_count, param_count);
        }
        if (existing->return_type != return_type) {
            PARSER_ERROR(parser,
                    "error: function '%s' return type mismatch\n", name);
        }
        if (!existing->has_no_prototype) {
            for (int i = 0; i < param_count; i++) {
                if (existing->param_types[i] != param_types[i]) {
                    PARSER_ERROR(parser,
                            "error: function '%s' parameter type mismatch at position %d\n",
                            name, i + 1);
                }
            }
        }
        if (is_definition) {
            if (existing->has_definition) {
                PARSER_ERROR(parser,
                        "error: duplicate function definition '%s'\n",
                        name);
            }
            existing->has_definition = 1;
        }
        return;
    }
    if (param_count > FUNC_MAX_PARAMS) {
        PARSER_ERROR(parser,
                "error: function '%s' has %d parameters; max supported is %d\n",
                name, param_count, FUNC_MAX_PARAMS);
    }
    {
        FuncSig sig;
        memset(&sig, 0, sizeof(sig));
        sig.name = name;
        sig.param_count = param_count;
        sig.has_definition = is_definition;
        sig.return_type = return_type;
        sig.storage_class = sc;
        sig.is_variadic = is_variadic;
        sig.has_no_prototype = is_no_prototype;
        for (int i = 0; i < param_count; i++) {
            sig.param_types[i] = param_types[i];
        }
        vec_push(&parser->funcs, &sig);
    }
    parser->func_count++;
}

static Token parser_expect(Parser *parser, TokenType type) {
    if (parser->current.type != type) {
        PARSER_ERROR(parser, "error: expected %s, got %s ('%s')\n",
                token_display_name(type),
                token_display_name(parser->current.type),
                parser->current.value);
    }
    Token token = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    return token;
}

/* Forward declarations needed by parse_struct_specifier. */
static Type *parse_type_specifier(Parser *parser, TypeKind *out_kind);
static ParsedDeclarator parse_declarator(Parser *parser);
static long eval_const_expr(Parser *parser, const char *context);
static long eval_const_unary(Parser *parser, const char *context);

/* Stage 30: struct tag table helpers. */
static StructTag *parser_find_struct_tag(Parser *parser, const char *tag) {
    for (size_t i = 0; i < parser->struct_tags.len; i++) {
        StructTag *st = (StructTag *)vec_get(&parser->struct_tags, i);
        if (strcmp(st->tag, tag) == 0)
            return st;
    }
    return NULL;
}

static StructTag *parser_register_struct_tag(Parser *parser, const char *tag) {
    StructTag *st = parser_find_struct_tag(parser, tag);
    if (st) return st;
    StructTag new_st;
    new_st.tag = tag;
    new_st.type = NULL;
    vec_push(&parser->struct_tags, &new_st);
    return (StructTag *)vec_get(&parser->struct_tags, parser->struct_tags.len - 1);
}

/*
 * Stage 86: build a (possibly multi-dimensional) array Type from a list of
 * dimensions. dims[0] is the outermost (leftmost) dimension. The type is
 * constructed inside-out: start with elem_type, wrap each dimension from
 * right (innermost) to left (outermost).
 *
 * Example: int table[4][8] → dims=[4,8], produces array[4] of array[8] of int.
 */
static Type *build_array_type_from_dims(Type *elem_type, int *dims, int dim_count) {
    Type *t = elem_type;
    for (int i = dim_count - 1; i >= 0; i--)
        t = type_array(t, dims[i]);
    return t;
}

/*
 * <struct_specifier> ::= "struct" [ <identifier> ] "{" <struct_declaration_list> "}"
 *                      | "struct" <identifier>
 *
 * <struct_declaration_list> ::= <struct_declaration> { <struct_declaration> }
 * <struct_declaration>      ::= <type_specifier> <struct_declarator_list> ";"
 * <struct_declarator_list>  ::= <declarator> { "," <declarator> }
 *
 * Stage 73-01: the tag is optional when a body '{' follows (anonymous struct).
 * Each anonymous definition creates a unique Type*; type identity is by pointer.
 * Layout uses natural alignment: each field is aligned to its type's natural
 * alignment; the struct's total size is rounded up to the struct's alignment
 * (the maximum field alignment). Returns the TYPE_STRUCT Type*.
 */
static Type *parse_struct_specifier(Parser *parser) {
    /* consume 'struct' */
    parser->current = lexer_next_token(parser->lexer);

    int has_tag = (parser->current.type == TOKEN_IDENTIFIER);
    const char *tag = NULL;
    StructTag *st = NULL;

    if (has_tag) {
        tag = parser->current.value;
        parser->current = lexer_next_token(parser->lexer);
        st = parser_register_struct_tag(parser, tag);
    } else if (parser->current.type != TOKEN_LBRACE) {
        PARSER_ERROR(parser, "error: expected identifier or '{' after 'struct'\n");
    }

    if (parser->current.type == TOKEN_LBRACE) {
        parser->current = lexer_next_token(parser->lexer);

        int current_offset = 0;
        int max_align = 1;

        /* Stage 31: collect field descriptors while parsing.
         * Stage 95-06: converted from fixed stack array to Vec. */
        Vec tmp_fields_vec;
        vec_init(&tmp_fields_vec, sizeof(StructField));

        /* Stage 134: bit-field packing state.
         * bf_unit_offset: byte offset of the current storage unit (-1 = none).
         * bf_unit_size:   size of current storage unit in bytes.
         * bf_bits_used:   bits already consumed in the current unit. */
        int bf_unit_offset = -1;
        int bf_unit_size   = 0;
        int bf_bits_used   = 0;
        /* Stage 134: flexible array member flag (must be last named member). */
        int has_flexible_array = 0;

        while (parser->current.type != TOKEN_RBRACE) {
            /* Stage 134: a flexible array member must be the last member. */
            if (has_flexible_array) {
                PARSER_ERROR(parser,
                        "error: flexible array member must be the last named member\n");
            }
            /* Stage 82-01: consume optional leading const qualifier.
             * Stage 82-04: also consume optional volatile qualifier. */
            int field_is_const = 0;
            int field_is_volatile = 0;
            if (parser->current.type == TOKEN_CONST) {
                field_is_const = 1;
                parser->current = lexer_next_token(parser->lexer);
            } else if (parser->current.type == TOKEN_VOLATILE) {
                field_is_volatile = 1;
                parser->current = lexer_next_token(parser->lexer);
            }
            /* Parse field type specifier. */
            Type *field_base = parse_type_specifier(parser, NULL);

            /* Stage 159: GCC/C11 anonymous struct/union member — struct or union
             * type specifier with no following declarator (e.g. 'union { ... };'
             * embedded in another struct).  Skip it silently: members are not
             * added to the outer type, but the type itself is already parsed. */
            if (parser->current.type == TOKEN_SEMICOLON &&
                (field_base->kind == TYPE_STRUCT || field_base->kind == TYPE_UNION)) {
                parser->current = lexer_next_token(parser->lexer); /* consume ';' */
                continue;
            }

            /* Parse one or more declarators: each names a field.
             * Stage 134: anonymous bit-fields have no declarator — detect ':' early. */
            int first_in_list = 1;
            do {
                if (!first_in_list)
                    parser->current = lexer_next_token(parser->lexer); /* consume ',' */
                first_in_list = 0;

                /* Stage 134: anonymous bit-field: no name, just ': N'. */
                if (parser->current.type == TOKEN_COLON) {
                    parser->current = lexer_next_token(parser->lexer);
                    long bw = eval_const_expr(parser, "bit-field width");
                    if (bw < 0) {
                        PARSER_ERROR(parser, "error: negative bit-field width\n");
                    }
                    int bit_width = (int)bw;
                    int unit_size = type_size(field_base);
                    if (unit_size < 1) unit_size = 4; /* default to int-sized unit */
                    int unit_align = type_alignment(field_base);
                    if (unit_align < 1) unit_align = unit_size;
                    if (bit_width == 0) {
                        /* Zero-width: close the current storage unit. */
                        bf_unit_offset = -1;
                        bf_bits_used   = 0;
                        bf_unit_size   = 0;
                    } else {
                        /* Allocate a new unit if needed. */
                        if (bf_unit_offset < 0 || bf_unit_size != unit_size ||
                            bf_bits_used + bit_width > unit_size * 8) {
                            if (unit_align > max_align) max_align = unit_align;
                            current_offset = (current_offset + unit_align - 1)
                                             & ~(unit_align - 1);
                            bf_unit_offset  = current_offset;
                            bf_unit_size    = unit_size;
                            bf_bits_used    = 0;
                            current_offset += unit_size;
                        }
                        bf_bits_used += bit_width;
                        /* Anonymous bit-fields are not added to the field table. */
                    }
                    continue; /* next declarator in list or end of do-while */
                }

                ParsedDeclarator d = parse_declarator(parser);

                /* Stage 134: named bit-field: declarator followed by ':'. */
                if (parser->current.type == TOKEN_COLON) {
                    parser->current = lexer_next_token(parser->lexer);
                    long bw = eval_const_expr(parser, "bit-field width");
                    if (bw <= 0) {
                        PARSER_ERROR(parser,
                                "error: named bit-field '%s' must have positive width\n",
                                d.name ? d.name : "(unnamed)");
                    }
                    int bit_width = (int)bw;
                    int unit_size = type_size(field_base);
                    if (unit_size < 1) unit_size = 4;
                    int unit_align = type_alignment(field_base);
                    if (unit_align < 1) unit_align = unit_size;
                    if (bit_width > unit_size * 8) {
                        PARSER_ERROR(parser,
                                "error: bit-field width %d exceeds storage unit size\n",
                                bit_width);
                    }
                    /* Open a new storage unit if the field doesn't fit. */
                    if (bf_unit_offset < 0 || bf_unit_size != unit_size ||
                        bf_bits_used + bit_width > unit_size * 8) {
                        if (unit_align > max_align) max_align = unit_align;
                        current_offset = (current_offset + unit_align - 1)
                                         & ~(unit_align - 1);
                        bf_unit_offset  = current_offset;
                        bf_unit_size    = unit_size;
                        bf_bits_used    = 0;
                        current_offset += unit_size;
                    }
                    {
                        StructField sf;
                        memset(&sf, 0, sizeof(sf));
                        sf.name        = d.name;
                        sf.offset      = bf_unit_offset;
                        sf.kind        = field_base->kind;
                        sf.is_const    = field_is_const ? 1 : 0;
                        sf.is_volatile = field_is_volatile ? 1 : 0;
                        sf.is_bitfield = 1;
                        sf.bit_width   = bit_width;
                        sf.bit_offset  = bf_bits_used;
                        vec_push(&tmp_fields_vec, &sf);
                    }
                    bf_bits_used += bit_width;
                    continue;
                }

                /* Regular (non-bit-field) field: close any open bit-field unit. */
                bf_unit_offset = -1;
                bf_unit_size   = 0;
                bf_bits_used   = 0;

                /* Stage 82-01: const base for pointer-to-const (e.g. const T *f).
                 * Stage 82-04: volatile base for pointer-to-volatile. */
                Type *effective_base = field_base;
                if (field_is_const && d.pointer_count > 0)
                    effective_base = type_const_copy(field_base);
                else if (field_is_volatile && d.pointer_count > 0)
                    effective_base = type_volatile_copy(field_base);
                /* Build the full field type from base + pointer stars. */
                Type *field_type = effective_base;
                for (int i = 0; i < d.pointer_count; i++)
                    field_type = type_pointer(field_type);
                /* Stage 78: handle array member fields (e.g. int values[3]).
                 * Stage 86: multi-dimensional array members.
                 * Stage 134: flexible array member — last unsized array in struct. */
                if (d.is_array) {
                    if (!d.has_size) {
                        /* Stage 134: C99 flexible array member (§6.7.2.1p16).
                         * Allowed only as the last member of a struct that has
                         * at least one other named member. */
                        if (tmp_fields_vec.len == 0) {
                            PARSER_ERROR(parser,
                                    "error: flexible array member '%s' may not be the"
                                    " only member of a struct\n", d.name);
                        }
                        /* Store as TYPE_ARRAY with length 0 (flexible). */
                        field_type = type_array(field_type, 0);
                        int falign = type_alignment(field_base);
                        if (falign < 1) falign = 1;
                        if (falign > max_align) max_align = falign;
                        current_offset = (current_offset + falign - 1) & ~(falign - 1);
                        {
                            StructField sf;
                            memset(&sf, 0, sizeof(sf));
                            sf.name             = d.name;
                            sf.offset           = current_offset;
                            sf.kind             = TYPE_ARRAY;
                            sf.full_type        = field_type;
                            sf.is_const         = field_is_const ? 1 : 0;
                            sf.is_volatile      = field_is_volatile ? 1 : 0;
                            sf.is_flexible_array = 1;
                            vec_push(&tmp_fields_vec, &sf);
                        }
                        /* Flexible array contributes 0 bytes to sizeof. */
                        has_flexible_array = 1;
                        continue;
                    }
                    field_type = build_array_type_from_dims(field_type,
                                                            d.array_dims, d.array_dim_count);
                }

                /* Stage 37/72: a non-pointer field of an incomplete struct or
                 * union type is invalid; pointer-to-incomplete is allowed. */
                if (field_type->kind == TYPE_STRUCT && field_type->size == 0) {
                    PARSER_ERROR(parser,
                            "error: field '%s' has incomplete struct type\n", d.name);
                }
                if (field_type->kind == TYPE_UNION && field_type->size == 0) {
                    PARSER_ERROR(parser,
                            "error: field '%s' has incomplete union type\n", d.name);
                }

                int fsz   = type_size(field_type);
                int falign = type_alignment(field_type);
                if (falign < 1) falign = 1;
                if (fsz < 1)    fsz = 1;

                if (falign > max_align) max_align = falign;
                /* Advance offset to satisfy field alignment. */
                current_offset = (current_offset + falign - 1) & ~(falign - 1);

                {
                    StructField sf;
                    memset(&sf, 0, sizeof(sf));
                    sf.name = d.name;
                    sf.offset    = current_offset;
                    sf.kind      = field_type->kind;
                    sf.full_type = (field_type->kind == TYPE_POINTER ||
                                   field_type->kind == TYPE_ARRAY   ||
                                   field_type->kind == TYPE_STRUCT  ||
                                   field_type->kind == TYPE_UNION)
                                   ? field_type : NULL;
                    /* Stage 82-01: const scalar member or const-pointer member. */
                    sf.is_const  =
                        ((field_is_const && d.pointer_count == 0 && !d.is_array) ||
                         d.pointer_is_const) ? 1 : 0;
                    /* Stage 82-04: volatile scalar member. */
                    sf.is_volatile =
                        (field_is_volatile && d.pointer_count == 0 && !d.is_array) ? 1 : 0;
                    vec_push(&tmp_fields_vec, &sf);
                }
                current_offset += fsz;

            } while (parser->current.type == TOKEN_COMMA);

            parser_expect(parser, TOKEN_SEMICOLON);
        }
        parser_expect(parser, TOKEN_RBRACE);

        /* Round total size up to the struct's alignment. */
        int total_size = (current_offset + max_align - 1) & ~(max_align - 1);
        if (total_size == 0) total_size = 1; /* empty struct: 1 byte by convention */

        Type *result;
        if (has_tag) {
            /* Re-fetch st: parsing nested type specifiers in the body may have
             * reallocated struct_tags, invalidating the pointer obtained before
             * the body was entered. */
            st = parser_find_struct_tag(parser, tag);
            /* Stage 37: if a placeholder was created by a forward declaration,
             * update it in-place so any existing Type* references (e.g. typedef
             * entries) automatically reflect the completed layout. */
            if (st->type && st->type->size == 0) {
                st->type->size      = total_size;
                st->type->alignment = max_align;
                result = st->type;
            } else {
                st->type = type_struct(total_size, max_align);
                result = st->type;
            }
        } else {
            /* Stage 73-01: anonymous struct — fresh unique Type* each time. */
            result = type_struct(total_size, max_align);
        }

        /* Stage 31: copy collected fields into the Type.
         * Stage 95-06: copy from Vec backing store. */
        if (tmp_fields_vec.len > 0) {
            int n = (int)tmp_fields_vec.len;
            result->fields = calloc((size_t)n, sizeof(StructField));
            memcpy(result->fields, tmp_fields_vec.data,
                   (size_t)n * sizeof(StructField));
            result->field_count = n;
        }
        vec_free(&tmp_fields_vec);

        return result;

    } else {
        /* Reference without body: "struct Tag" used as a type (e.g. pointer
         * target or forward declaration).  Stage 37: create an incomplete
         * placeholder so forward declarations and opaque pointer fields work.
         * has_tag is guaranteed true here (checked above). */
        if (!st->type)
            st->type = type_struct(0, 0);
        return st->type;
    }
}

/* Stage 72: union tag table helpers. */
static UnionTag *parser_find_union_tag(Parser *parser, const char *tag) {
    for (size_t i = 0; i < parser->union_tags.len; i++) {
        UnionTag *ut = (UnionTag *)vec_get(&parser->union_tags, i);
        if (strcmp(ut->tag, tag) == 0)
            return ut;
    }
    return NULL;
}

static UnionTag *parser_register_union_tag(Parser *parser, const char *tag) {
    UnionTag *ut = parser_find_union_tag(parser, tag);
    if (ut) return ut;
    UnionTag new_ut;
    new_ut.tag = tag;
    new_ut.type = NULL;
    vec_push(&parser->union_tags, &new_ut);
    return (UnionTag *)vec_get(&parser->union_tags, parser->union_tags.len - 1);
}

/*
 * <union_specifier> ::= "union" [ <identifier> ] "{" <struct_declaration_list> "}"
 *                     | "union" <identifier>
 *
 * Stage 73-01: the tag is optional when a body '{' follows (anonymous union).
 * Each anonymous definition creates a unique Type*; type identity is by pointer.
 * Layout: all member offsets are 0; size = max(member sizes) rounded up to
 * the union's alignment (max of all member alignments).
 * Returns the TYPE_UNION Type*.
 */
static Type *parse_union_specifier(Parser *parser) {
    /* consume 'union' */
    parser->current = lexer_next_token(parser->lexer);

    int has_tag = (parser->current.type == TOKEN_IDENTIFIER);
    const char *tag = NULL;
    UnionTag *ut = NULL;

    if (has_tag) {
        tag = parser->current.value;
        parser->current = lexer_next_token(parser->lexer);
        ut = parser_register_union_tag(parser, tag);
    } else if (parser->current.type != TOKEN_LBRACE) {
        PARSER_ERROR(parser, "error: expected identifier or '{' after 'union'\n");
    }

    if (parser->current.type == TOKEN_LBRACE) {
        parser->current = lexer_next_token(parser->lexer);

        int max_size  = 0;
        int max_align = 1;

        /* Stage 95-06: converted from fixed stack array to Vec. */
        Vec tmp_fields_vec;
        vec_init(&tmp_fields_vec, sizeof(StructField));

        while (parser->current.type != TOKEN_RBRACE) {
            /* Stage 82-01: consume optional leading const qualifier.
             * Stage 82-04: also consume optional volatile qualifier. */
            int field_is_const = 0;
            int field_is_volatile = 0;
            if (parser->current.type == TOKEN_CONST) {
                field_is_const = 1;
                parser->current = lexer_next_token(parser->lexer);
            } else if (parser->current.type == TOKEN_VOLATILE) {
                field_is_volatile = 1;
                parser->current = lexer_next_token(parser->lexer);
            }
            Type *field_base = parse_type_specifier(parser, NULL);

            /* Stage 159: anonymous union/struct member — no declarator follows. */
            if (parser->current.type == TOKEN_SEMICOLON &&
                (field_base->kind == TYPE_STRUCT || field_base->kind == TYPE_UNION)) {
                parser->current = lexer_next_token(parser->lexer);
                continue;
            }

            do {
                if (parser->current.type == TOKEN_COMMA)
                    parser->current = lexer_next_token(parser->lexer);

                ParsedDeclarator d = parse_declarator(parser);
                /* Stage 82-01: const base for pointer-to-const (e.g. const T *f).
                 * Stage 82-04: volatile base for pointer-to-volatile. */
                Type *effective_base = field_base;
                if (field_is_const && d.pointer_count > 0)
                    effective_base = type_const_copy(field_base);
                else if (field_is_volatile && d.pointer_count > 0)
                    effective_base = type_volatile_copy(field_base);
                Type *field_type = effective_base;
                for (int i = 0; i < d.pointer_count; i++)
                    field_type = type_pointer(field_type);
                /* Stage 78: handle array member fields (e.g. int values[3]). */
                if (d.is_array) {
                    if (!d.has_size) {
                        PARSER_ERROR(parser,
                                "error: union array member '%s' requires explicit size\n",
                                d.name);
                    }
                    /* Stage 86: multi-dimensional array members. */
                    field_type = build_array_type_from_dims(field_type,
                                                            d.array_dims, d.array_dim_count);
                }

                /* Reject non-pointer field of an incomplete struct or union type. */
                if ((field_type->kind == TYPE_STRUCT || field_type->kind == TYPE_UNION) &&
                    field_type->size == 0) {
                    PARSER_ERROR(parser,
                            "error: field '%s' has incomplete type\n", d.name);
                }

                int fsz   = type_size(field_type);
                int falign = type_alignment(field_type);
                if (falign < 1) falign = 1;
                if (fsz < 1)    fsz = 1;

                if (falign > max_align) max_align = falign;
                if (fsz > max_size)     max_size  = fsz;

                {
                    StructField sf;
                    memset(&sf, 0, sizeof(sf));
                    sf.name = d.name;
                    sf.offset    = 0; /* all union members at offset 0 */
                    sf.kind      = field_type->kind;
                    sf.full_type = (field_type->kind == TYPE_POINTER ||
                                   field_type->kind == TYPE_ARRAY   ||
                                   field_type->kind == TYPE_STRUCT  ||
                                   field_type->kind == TYPE_UNION)
                                   ? field_type : NULL;
                    /* Stage 82-01: const scalar member or const-pointer member. */
                    sf.is_const  =
                        ((field_is_const && d.pointer_count == 0 && !d.is_array) ||
                         d.pointer_is_const) ? 1 : 0;
                    /* Stage 82-04: volatile scalar member. */
                    sf.is_volatile =
                        (field_is_volatile && d.pointer_count == 0 && !d.is_array) ? 1 : 0;
                    vec_push(&tmp_fields_vec, &sf);
                }

            } while (parser->current.type == TOKEN_COMMA);

            parser_expect(parser, TOKEN_SEMICOLON);
        }
        parser_expect(parser, TOKEN_RBRACE);

        /* Round total size up to the union's alignment. */
        int total_size = (max_size + max_align - 1) & ~(max_align - 1);
        if (total_size == 0) total_size = 1;

        Type *result;
        if (has_tag) {
            /* Re-fetch ut: parsing nested type specifiers in the body may have
             * reallocated union_tags, invalidating the pointer obtained before
             * the body was entered. */
            ut = parser_find_union_tag(parser, tag);
            if (ut->type && ut->type->size == 0) {
                ut->type->size      = total_size;
                ut->type->alignment = max_align;
                result = ut->type;
            } else {
                ut->type = type_union(total_size, max_align);
                result = ut->type;
            }
        } else {
            /* Stage 73-01: anonymous union — fresh unique Type* each time. */
            result = type_union(total_size, max_align);
        }

        if (tmp_fields_vec.len > 0) {
            int n = (int)tmp_fields_vec.len;
            result->fields = calloc((size_t)n, sizeof(StructField));
            memcpy(result->fields, tmp_fields_vec.data,
                   (size_t)n * sizeof(StructField));
            result->field_count = n;
        }
        vec_free(&tmp_fields_vec);

        return result;

    } else {
        /* Forward declaration / reference without body.
         * has_tag is guaranteed true here (checked above). */
        if (!ut->type)
            ut->type = type_union(0, 0);
        return ut->type;
    }
}

static long eval_const_expr(Parser *parser, const char *context);

/*
 * <enum_specifier> ::= "enum" <identifier> "{" <enumerator_list> "}"
 *                    | "enum"             "{" <enumerator_list> "}"
 *                    | "enum" <identifier>
 *
 * <enumerator_list> ::= <enumerator> { "," <enumerator> } [","]
 * <enumerator>      ::= <identifier> [ "=" <integer_literal> | <char_literal> ]
 *
 * Always returns type_int(). Enum constants are registered in the parser's
 * flat enum_consts table so parse_primary can fold them to AST_INT_LITERAL.
 */
static Type *parse_enum_specifier(Parser *parser) {
    /* consume 'enum' */
    parser->current = lexer_next_token(parser->lexer);

    const char *tag = NULL;
    int has_tag = 0;

    if (parser->current.type == TOKEN_IDENTIFIER) {
        tag = parser->current.value;
        has_tag = 1;
        parser->current = lexer_next_token(parser->lexer);
    }

    if (parser->current.type == TOKEN_LBRACE) {
        parser->current = lexer_next_token(parser->lexer);

        if (has_tag) {
            /* Register or update the tag as defined. */
            EnumTag *et = NULL;
            for (size_t i = 0; i < parser->enum_tags.len; i++) {
                EnumTag *t = (EnumTag *)vec_get(&parser->enum_tags, i);
                if (strcmp(t->tag, tag) == 0) {
                    et = t;
                    break;
                }
            }
            if (!et) {
                EnumTag new_et;
                new_et.tag = tag;
                new_et.is_defined = 0;
                vec_push(&parser->enum_tags, &new_et);
                et = (EnumTag *)vec_get(&parser->enum_tags,
                                        parser->enum_tags.len - 1);
            }
            et->is_defined = 1;
        }

        long next_val = 0;

        while (parser->current.type != TOKEN_RBRACE) {
            if (parser->current.type != TOKEN_IDENTIFIER) {
                PARSER_ERROR(parser, "error: expected identifier in enumerator list, got '%s'\n",
                        parser->current.value);
            }
            const char *ename = parser->current.value;
            parser->current = lexer_next_token(parser->lexer);

            for (size_t i = 0; i < parser->enum_consts.len; i++) {
                EnumConst *ec = (EnumConst *)vec_get(&parser->enum_consts, i);
                if (strcmp(ec->name, ename) == 0) {
                    PARSER_ERROR(parser, "error: duplicate enumerator '%s'\n", ename);
                }
            }

            if (parser->current.type == TOKEN_ASSIGN) {
                parser->current = lexer_next_token(parser->lexer);
                next_val = eval_const_expr(parser, "enumerator value");
            }

            EnumConst new_ec;
            new_ec.name = ename;
            new_ec.value = next_val++;
            vec_push(&parser->enum_consts, &new_ec);

            if (parser->current.type == TOKEN_COMMA) {
                parser->current = lexer_next_token(parser->lexer);
                /* trailing comma allowed — if next is '}' the while exits */
            }
        }
        parser_expect(parser, TOKEN_RBRACE);

    } else {
        /* Enum reference without body: "enum Tag" */
        if (!has_tag) {
            PARSER_ERROR(parser, "error: expected identifier or '{' after 'enum'\n");
        }
        int found = 0;
        for (size_t i = 0; i < parser->enum_tags.len; i++) {
            EnumTag *t = (EnumTag *)vec_get(&parser->enum_tags, i);
            if (strcmp(t->tag, tag) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            /* Tag not found and no body — create a forward-declaration entry. */
            EnumTag new_et;
            new_et.tag        = tag;
            new_et.is_defined = 0;
            vec_push(&parser->enum_tags, &new_et);
        }
    }

    return type_int();
}

/*
 * <type_specifier> ::= "void" | "char" | "short" | "int" | "long"
 *                    | <typedef_name> | <enum_specifier> | <struct_specifier>
 *
 * Parses one type keyword, advances the token, and returns the
 * corresponding Type*. Writes the TypeKind to *out_kind when non-NULL.
 */
static Type *parse_type_specifier(Parser *parser, TypeKind *out_kind) {
    TypeKind kind;
    Type *t;
    switch (parser->current.type) {
    case TOKEN_VOID:   kind = TYPE_VOID;   t = type_void();   break;
    case TOKEN_BOOL:   kind = TYPE_BOOL;   t = type_bool();   break;
    case TOKEN_CHAR:   kind = TYPE_CHAR;   t = type_char();   break;
    case TOKEN_SHORT:  kind = TYPE_SHORT;  t = type_short();  break;
    case TOKEN_FLOAT: {
        parser->current = lexer_next_token(parser->lexer);
        if (out_kind) *out_kind = TYPE_FLOAT;
        return type_float();
    }
    case TOKEN_DOUBLE: {
        parser->current = lexer_next_token(parser->lexer);
        if (out_kind) *out_kind = TYPE_DOUBLE;
        return type_double();
    }
    case TOKEN_LONG: {
        /* Stage 64: "long long" — peek at next token for second 'long'. */
        parser->current = lexer_next_token(parser->lexer);
        /* "long unsigned ..." → delegate to unsigned path */
        if (parser->current.type == TOKEN_UNSIGNED) {
            parser->current = lexer_next_token(parser->lexer);
            if (parser->current.type == TOKEN_LONG) {
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_LONG) {
                    PARSER_ERROR(parser, "error: 'long long long' is not a valid type\n");
                }
                if (parser->current.type == TOKEN_INT)
                    parser->current = lexer_next_token(parser->lexer);
                if (out_kind) *out_kind = TYPE_UNSIGNED_LONG_LONG;
                return type_unsigned_long_long();
            }
            if (parser->current.type == TOKEN_INT)
                parser->current = lexer_next_token(parser->lexer);
            if (out_kind) *out_kind = TYPE_LONG;
            return type_unsigned_long();
        }
        /* "long signed ..." → signed long */
        if (parser->current.type == TOKEN_SIGNED) {
            parser->current = lexer_next_token(parser->lexer);
            if (parser->current.type == TOKEN_LONG) {
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_LONG) {
                    PARSER_ERROR(parser, "error: 'long long long' is not a valid type\n");
                }
                if (parser->current.type == TOKEN_INT)
                    parser->current = lexer_next_token(parser->lexer);
                if (out_kind) *out_kind = TYPE_LONG_LONG;
                return type_long_long();
            }
            if (parser->current.type == TOKEN_INT)
                parser->current = lexer_next_token(parser->lexer);
            if (out_kind) *out_kind = TYPE_LONG;
            return type_long();
        }
        if (parser->current.type == TOKEN_LONG) {
            parser->current = lexer_next_token(parser->lexer);
            if (parser->current.type == TOKEN_LONG) {
                PARSER_ERROR(parser, "error: 'long long long' is not a valid type\n");
            }
            /* "long long unsigned [int]" */
            if (parser->current.type == TOKEN_UNSIGNED) {
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_INT)
                    parser->current = lexer_next_token(parser->lexer);
                if (out_kind) *out_kind = TYPE_UNSIGNED_LONG_LONG;
                return type_unsigned_long_long();
            }
            /* "long long signed [int]" */
            if (parser->current.type == TOKEN_SIGNED) {
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_INT)
                    parser->current = lexer_next_token(parser->lexer);
                if (out_kind) *out_kind = TYPE_LONG_LONG;
                return type_long_long();
            }
            if (parser->current.type == TOKEN_INT)
                parser->current = lexer_next_token(parser->lexer);
            if (out_kind) *out_kind = TYPE_LONG_LONG;
            return type_long_long();
        }
        /* "long double" — treat as double for codegen purposes */
        if (parser->current.type == TOKEN_DOUBLE) {
            parser->current = lexer_next_token(parser->lexer);
            if (out_kind) *out_kind = TYPE_DOUBLE;
            return type_double();
        }
        /* single "long" — optional trailing "int" consumed below */
        kind = TYPE_LONG;  t = type_long();
        if (parser->current.type == TOKEN_INT)
            parser->current = lexer_next_token(parser->lexer);
        if (out_kind) *out_kind = kind;
        return t;
    }
    case TOKEN_INT:   kind = TYPE_INT;   t = type_int();   break;
    case TOKEN_SIGNED: {
        /* Stage 61: "signed" [ "char" | "short" [ "int" ] | "int" | "long" [ "int" ] ]
         * signed → int, signed char → char, signed short → short,
         * signed int → int, signed long → long */
        parser->current = lexer_next_token(parser->lexer); /* consume 'signed' */
        if (parser->current.type == TOKEN_UNSIGNED) {
            PARSER_ERROR(parser, 
                    "error: 'signed' and 'unsigned' cannot both be specified\n");
        }
        if (parser->current.type == TOKEN_BOOL) {
            PARSER_ERROR(parser, 
                    "error: _Bool type cannot have a sign qualifier\n");
        }
        switch (parser->current.type) {
        case TOKEN_CHAR:
            kind = TYPE_CHAR;  t = type_char();
            break;
        case TOKEN_SHORT:
            kind = TYPE_SHORT; t = type_short();
            parser->current = lexer_next_token(parser->lexer);
            if (parser->current.type == TOKEN_INT)
                parser->current = lexer_next_token(parser->lexer);
            if (out_kind) *out_kind = kind;
            return t;
        case TOKEN_INT:
            kind = TYPE_INT;   t = type_int();
            break;
        case TOKEN_LONG:
            /* Stage 64: "signed long long [int]" */
            parser->current = lexer_next_token(parser->lexer);
            if (parser->current.type == TOKEN_LONG) {
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_LONG) {
                    PARSER_ERROR(parser, "error: 'long long long' is not a valid type\n");
                }
                if (parser->current.type == TOKEN_INT)
                    parser->current = lexer_next_token(parser->lexer);
                if (out_kind) *out_kind = TYPE_LONG_LONG;
                return type_long_long();
            }
            if (parser->current.type == TOKEN_INT)
                parser->current = lexer_next_token(parser->lexer);
            if (out_kind) *out_kind = TYPE_LONG;
            return type_long();
        default:
            /* plain "signed" == int; do not consume the next token */
            kind = TYPE_INT;   t = type_int();
            if (out_kind) *out_kind = kind;
            return t;
        }
        /* Advance past the type keyword. */
        parser->current = lexer_next_token(parser->lexer);
        if (out_kind) *out_kind = kind;
        return t;
    }
    case TOKEN_UNSIGNED: {
        /* Stage 40: "unsigned" [ "char" | "short" [ "int" ] | "int" | "long" [ "int" ] ]
         * Stage 61: also consume optional trailing "int" for short/long. */
        parser->current = lexer_next_token(parser->lexer); /* consume 'unsigned' */
        if (parser->current.type == TOKEN_SIGNED) {
            PARSER_ERROR(parser, 
                    "error: 'signed' and 'unsigned' cannot both be specified\n");
        }
        if (parser->current.type == TOKEN_BOOL) {
            PARSER_ERROR(parser, 
                    "error: _Bool type cannot have a sign qualifier\n");
        }
        switch (parser->current.type) {
        case TOKEN_CHAR:
            kind = TYPE_CHAR;  t = type_unsigned_char();
            break;
        case TOKEN_SHORT:
            kind = TYPE_SHORT; t = type_unsigned_short();
            parser->current = lexer_next_token(parser->lexer);
            if (parser->current.type == TOKEN_INT)
                parser->current = lexer_next_token(parser->lexer);
            if (out_kind) *out_kind = kind;
            return t;
        case TOKEN_INT:
            kind = TYPE_INT;   t = type_unsigned_int();
            break;
        case TOKEN_LONG:
            /* Stage 64: "unsigned long long [int]" */
            parser->current = lexer_next_token(parser->lexer);
            if (parser->current.type == TOKEN_LONG) {
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_LONG) {
                    PARSER_ERROR(parser, "error: 'long long long' is not a valid type\n");
                }
                if (parser->current.type == TOKEN_INT)
                    parser->current = lexer_next_token(parser->lexer);
                if (out_kind) *out_kind = TYPE_UNSIGNED_LONG_LONG;
                return type_unsigned_long_long();
            }
            if (parser->current.type == TOKEN_INT)
                parser->current = lexer_next_token(parser->lexer);
            if (out_kind) *out_kind = TYPE_LONG;
            return type_unsigned_long();
        default:
            /* plain "unsigned" == unsigned int; do not consume the next token */
            kind = TYPE_INT;   t = type_unsigned_int();
            if (out_kind) *out_kind = kind;
            return t;
        }
        /* Advance past the type keyword. */
        parser->current = lexer_next_token(parser->lexer);
        if (out_kind) *out_kind = kind;
        return t;
    }
    case TOKEN_IDENTIFIER: {
        /* Stage 28-01/28-02: typedef name used as a type specifier. */
        TypedefEntry *entry = parser_find_typedef(parser, parser->current.value);
        if (!entry) {
            PARSER_ERROR(parser, "error: unknown type name '%s'\n",
                    parser->current.value);
        }
        kind = entry->kind;
        if (entry->full_type) {
            t = entry->full_type; /* pointer typedef or unsigned scalar typedef */
        } else {
            switch (kind) {
            case TYPE_CHAR:               t = type_char();               break;
            case TYPE_SHORT:              t = type_short();              break;
            case TYPE_LONG:               t = type_long();               break;
            case TYPE_LONG_LONG:          t = type_long_long();          break;
            case TYPE_UNSIGNED_LONG_LONG: t = type_unsigned_long_long(); break;
            default:                      t = type_int();                break;
            }
        }
        break;
    }
    case TOKEN_ENUM: {
        /* parse_enum_specifier advances past all enum tokens itself. */
        Type *et = parse_enum_specifier(parser);
        if (out_kind) *out_kind = TYPE_INT;
        return et;
    }
    case TOKEN_STRUCT: {
        /* parse_struct_specifier advances past all struct tokens itself. */
        Type *st = parse_struct_specifier(parser);
        if (out_kind) *out_kind = TYPE_STRUCT;
        return st;
    }
    case TOKEN_UNION: {
        /* parse_union_specifier advances past all union tokens itself. */
        Type *ut = parse_union_specifier(parser);
        if (out_kind) *out_kind = TYPE_UNION;
        return ut;
    }
    default:
        PARSER_ERROR(parser, "error: expected integer type, got '%s'\n",
                parser->current.value);
    }
    parser->current = lexer_next_token(parser->lexer);
    /* Stage 61: "short int" and "long int" — optional trailing 'int'. */
    if ((kind == TYPE_SHORT || kind == TYPE_LONG) && parser->current.type == TOKEN_INT)
        parser->current = lexer_next_token(parser->lexer);
    if (out_kind) *out_kind = kind;
    return t;
}

/*
 * <type_name> ::= <specifier_qualifier_list> [ <abstract_declarator> ]
 * <specifier_qualifier_list> ::= <specifier_qualifier> { <specifier_qualifier> }
 * <specifier_qualifier> ::= <type_specifier> | <sign_specifier> | <type_qualifier>
 * <abstract_declarator> ::= "*" { <type_qualifier> } { "*" { <type_qualifier> } }
 *
 * Nameless type context: cast expressions, sizeof(type), and va_arg.
 * Returns the fully pointer-wrapped Type*.
 */
static Type *parse_type_name(Parser *parser) {
    /* Stage 82-03: consume optional leading const qualifier.
     * Stage 82-04: also consume optional leading volatile qualifier. */
    int base_is_const = 0;
    int base_is_volatile = 0;
    if (parser->current.type == TOKEN_CONST) {
        base_is_const = 1;
        parser->current = lexer_next_token(parser->lexer);
    } else if (parser->current.type == TOKEN_VOLATILE) {
        base_is_volatile = 1;
        parser->current = lexer_next_token(parser->lexer);
    }
    /* Stage 40: optional leading unsigned/signed handled inside
     * parse_type_specifier when TOKEN_UNSIGNED/TOKEN_SIGNED is current. */
    Type *t = parse_type_specifier(parser, NULL);
    if (base_is_const)
        t = type_const_copy(t);
    else if (base_is_volatile)
        t = type_volatile_copy(t);
    /* Stage 82-03: abstract_pointer_declarator — each "*" may be followed
     * by optional "const" or "volatile" qualifiers (pointer-level). */
    while (parser->current.type == TOKEN_STAR) {
        parser->current = lexer_next_token(parser->lexer);
        /* Consume optional const/volatile/restrict after each star. */
        while (parser->current.type == TOKEN_CONST   ||
               parser->current.type == TOKEN_VOLATILE ||
               parser->current.type == TOKEN_RESTRICT)
            parser->current = lexer_next_token(parser->lexer);
        t = type_pointer(t);
    }
    /* Stage 86: optional array-suffix for sizeof(int[N]) and sizeof(int[N][M]).
     * Stage 98: the first dimension may be omitted (int[]) in the compound-literal
     * context; parse_cast detects this and routes to build_compound_literal.
     * Omitted first dimension is represented as length 0 in the returned type;
     * callers that disallow it (sizeof, cast, va_arg) check for length == 0. */
    if (parser->current.type == TOKEN_LBRACKET) {
        int dims[MAX_ARRAY_DIMS];
        int dim_count = 0;
        while (parser->current.type == TOKEN_LBRACKET) {
            if (dim_count >= MAX_ARRAY_DIMS) {
                PARSER_ERROR(parser,
                        "error: too many array dimensions (max %d)\n", MAX_ARRAY_DIMS);
            }
            parser->current = lexer_next_token(parser->lexer);
            if (dim_count == 0 && parser->current.type == TOKEN_RBRACKET) {
                /* Omitted first dimension: stage 98 compound literal (int[]). */
                parser->current = lexer_next_token(parser->lexer);
                dims[dim_count++] = 0;
                continue;
            }
            long length = eval_const_expr(parser, "array dimension");
            if (length <= 0) {
                PARSER_ERROR(parser, "error: array size must be greater than zero\n");
            }
            dims[dim_count++] = (int)length;
            parser_expect(parser, TOKEN_RBRACKET);
        }
        t = build_array_type_from_dims(t, dims, dim_count);
    }
    return t;
}

/*
 * <declarator>        ::= { "*" } <direct_declarator>
 * <direct_declarator> ::= <identifier>
 *                       | "(" <declarator> ")"
 *                       | <direct_declarator> "[" [<integer_literal>] "]"
 *                       | <direct_declarator> "(" [<parameter_list>] ")"
 *
 * For the function form on a plain identifier, is_function is set to 1
 * but the "(" is NOT consumed; the caller handles the parameter list.
 *
 * Stage 25-01 / Stage 26: the parenthesized form handles three cases:
 *   - "(" { "*" } identifier ")" "(" ... ")" with at least one inner star
 *     → function-pointer declarator: is_func_pointer=1.
 *     Stage 26: the trailing "(" param-list ")" is consumed here and
 *     stored in fp_param_types/fp_param_count; callers build Type* inline.
 *   - "(" identifier ")" "(" ... ")" with zero inner stars
 *     → redundant-paren function declarator: is_function=1 (valid C99).
 *   - "(" { "*" } identifier ")" [ "[" ... "]" ]
 *     → parenthesized pointer/array declarator: pointer_count carries
 *     outer+inner stars combined.
 *
 * fp_outer_stars: stars before the opening "(" — become part of the
 *   function's return type when is_func_pointer is set.
 * fp_inner_stars: stars inside "(*)".
 */
static ParsedDeclarator parse_declarator(Parser *parser) {
    ParsedDeclarator d;
    memset(&d, 0, sizeof(d));

    int outer_stars = 0;
    int pointer_is_const = 0;
    while (parser->current.type == TOKEN_STAR) {
        outer_stars++;
        parser->current = lexer_next_token(parser->lexer);
        /* Stage 66: consume optional "const" qualifier after each star.
         * Stage 82-04: also consume optional "volatile" qualifier.
         * Stage 106: also consume optional "restrict" qualifier.
         * The last const qualifier marks the pointer itself as const. */
        while (parser->current.type == TOKEN_CONST   ||
               parser->current.type == TOKEN_VOLATILE ||
               parser->current.type == TOKEN_RESTRICT) {
            if (parser->current.type == TOKEN_CONST)
                pointer_is_const = 1;
            /* volatile and restrict: consume and ignore */
            parser->current = lexer_next_token(parser->lexer);
        }
    }
    d.pointer_is_const = pointer_is_const;

    if (parser->current.type == TOKEN_LPAREN) {
        /* Parenthesized declarator: "(" { "*" } identifier [ suffix ] ")" */
        parser->current = lexer_next_token(parser->lexer); /* consume "(" */
        int inner_stars = 0;
        while (parser->current.type == TOKEN_STAR) {
            inner_stars++;
            parser->current = lexer_next_token(parser->lexer);
        }
        Token name = parser_expect(parser, TOKEN_IDENTIFIER);
        d.name = name.value;
        /* Stage 137: (*name())(params) — function returning pointer-to-function.
         * inner_stars > 0 makes the return type a pointer (valid in C99);
         * inner_stars == 0 would be a function returning a function type
         * directly, which is forbidden by C99. */
        if (parser->current.type == TOKEN_LPAREN) {
            if (inner_stars == 0) {
                PARSER_ERROR(parser,
                    "error: functions cannot return function types directly\n");
            }
            /* Parse the outer function's own parameter list, e.g. () of
             * get_adder() in  int (*get_adder())(int). */
            parser_expect(parser, TOKEN_LPAREN);
            int own_count = 0;
            int own_no_proto = 0;
            int own_is_void = 0;
            if (parser->current.type == TOKEN_VOID) {
                int sp = parser->lexer->pos;
                Token st = parser->current;
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_RPAREN)
                    own_is_void = 1;
                else { parser->lexer->pos = sp; parser->current = st; }
            }
            if (!own_is_void && parser->current.type != TOKEN_RPAREN) {
                while (own_count < FUNC_TYPE_MAX_PARAMS) {
                    while (parser->current.type == TOKEN_CONST   ||
                           parser->current.type == TOKEN_VOLATILE ||
                           parser->current.type == TOKEN_RESTRICT)
                        parser->current = lexer_next_token(parser->lexer);
                    Type *pt = parse_type_specifier(parser, NULL);
                    /* Stage 159: trailing qualifiers after base type (e.g. 'void const *'). */
                    while (parser->current.type == TOKEN_CONST   ||
                           parser->current.type == TOKEN_VOLATILE ||
                           parser->current.type == TOKEN_RESTRICT)
                        parser->current = lexer_next_token(parser->lexer);
                    int stars = 0;
                    while (parser->current.type == TOKEN_STAR) {
                        stars++;
                        parser->current = lexer_next_token(parser->lexer);
                    }
                    if (parser->current.type == TOKEN_IDENTIFIER)
                        parser->current = lexer_next_token(parser->lexer);
                    /* C99 §6.7.5.3p7: array param adjusted to pointer (e.g. char *argv[]). */
                    while (parser->current.type == TOKEN_LBRACKET) {
                        parser->current = lexer_next_token(parser->lexer);
                        while (parser->current.type != TOKEN_RBRACKET &&
                               parser->current.type != TOKEN_EOF)
                            parser->current = lexer_next_token(parser->lexer);
                        if (parser->current.type == TOKEN_RBRACKET)
                            parser->current = lexer_next_token(parser->lexer);
                        stars++;
                    }
                    int j;
                    for (j = 0; j < stars; j++) pt = type_pointer(pt);
                    d.own_param_types[own_count++] = pt;
                    if (parser->current.type != TOKEN_COMMA) break;
                    parser->current = lexer_next_token(parser->lexer);
                }
            } else if (!own_is_void) {
                own_no_proto = 1; /* empty () — no prototype */
            }
            parser_expect(parser, TOKEN_RPAREN); /* close own params */
            d.own_param_count = own_count;
            d.own_is_no_prototype = own_no_proto;
            /* Close the outer ')' of the parenthesized declarator. */
            parser_expect(parser, TOKEN_RPAREN);
            /* Parse the returned function pointer's parameter list, e.g. (int)
             * in  int (*get_adder())(int). */
            parser_expect(parser, TOKEN_LPAREN);
            int fp_count = 0;
            int fp_is_void = 0;
            if (parser->current.type == TOKEN_VOID) {
                int sp2 = parser->lexer->pos;
                Token st2 = parser->current;
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_RPAREN)
                    fp_is_void = 1;
                else { parser->lexer->pos = sp2; parser->current = st2; }
            }
            if (!fp_is_void && parser->current.type != TOKEN_RPAREN) {
                while (fp_count < FUNC_TYPE_MAX_PARAMS) {
                    while (parser->current.type == TOKEN_CONST   ||
                           parser->current.type == TOKEN_VOLATILE ||
                           parser->current.type == TOKEN_RESTRICT)
                        parser->current = lexer_next_token(parser->lexer);
                    Type *pt = parse_type_specifier(parser, NULL);
                    /* Stage 159: trailing qualifiers after base type. */
                    while (parser->current.type == TOKEN_CONST   ||
                           parser->current.type == TOKEN_VOLATILE ||
                           parser->current.type == TOKEN_RESTRICT)
                        parser->current = lexer_next_token(parser->lexer);
                    int stars = 0;
                    while (parser->current.type == TOKEN_STAR) {
                        stars++;
                        parser->current = lexer_next_token(parser->lexer);
                    }
                    if (parser->current.type == TOKEN_IDENTIFIER)
                        parser->current = lexer_next_token(parser->lexer);
                    /* C99 §6.7.5.3p7: array param adjusted to pointer (e.g. char *argv[]). */
                    while (parser->current.type == TOKEN_LBRACKET) {
                        parser->current = lexer_next_token(parser->lexer);
                        while (parser->current.type != TOKEN_RBRACKET &&
                               parser->current.type != TOKEN_EOF)
                            parser->current = lexer_next_token(parser->lexer);
                        if (parser->current.type == TOKEN_RBRACKET)
                            parser->current = lexer_next_token(parser->lexer);
                        stars++;
                    }
                    int j;
                    for (j = 0; j < stars; j++) pt = type_pointer(pt);
                    d.fp_param_types[fp_count++] = pt;
                    if (parser->current.type != TOKEN_COMMA) break;
                    parser->current = lexer_next_token(parser->lexer);
                }
            }
            parser_expect(parser, TOKEN_RPAREN); /* close fp params */
            d.fp_param_count = fp_count;
            d.is_func_returning_fp = 1;
            d.fp_inner_stars = inner_stars;
            d.fp_outer_stars = outer_stars;
            return d;
        }
        /* Optional array suffix inside parens: (*a[10]) */
        if (parser->current.type == TOKEN_LBRACKET) {
            d.is_array = 1;
            parser->current = lexer_next_token(parser->lexer);
            if (parser->current.type != TOKEN_RBRACKET) {
                long length = eval_const_expr(parser, "array dimension");
                if (length <= 0) {
                    PARSER_ERROR(parser, "error: array size must be greater than zero\n");
                }
                d.array_length = (int)length;
                d.has_size = 1;
            }
            parser_expect(parser, TOKEN_RBRACKET);
        }
        parser_expect(parser, TOKEN_RPAREN);
        /* Check suffix after the closing ")" */
        if (parser->current.type == TOKEN_LBRACKET) {
            /* Stage 135: C99 §6.7.5.2 pointer-to-array: int (*row)[4] or (*row)[]. */
            parser->current = lexer_next_token(parser->lexer); /* consume '[' */
            if (parser->current.type != TOKEN_RBRACKET) {
                long length = eval_const_expr(parser, "pointer-to-array bound");
                if (length <= 0)
                    PARSER_ERROR(parser,
                            "error: pointer-to-array bound must be positive\n");
                d.ptr_to_array_length = (int)length;
                d.ptr_to_array_has_size = 1;
            }
            parser_expect(parser, TOKEN_RBRACKET);
            d.is_ptr_to_array = 1;
            d.pointer_count = outer_stars + inner_stars;
            return d;
        }
        if (parser->current.type == TOKEN_LPAREN) {
            if (inner_stars > 0) {
                /* Stage 26: function-pointer declarator int (*fp)(...).
                 * Consume the parameter list inline; store in fp_param_types. */
                d.is_func_pointer = 1;
                d.fp_outer_stars  = outer_stars;
                d.fp_inner_stars  = inner_stars;
                parser_expect(parser, TOKEN_LPAREN);
                int count = 0;
                /* Stage 135: handle (void) as a zero-parameter list, matching
                 * the same rule used in function declarations. */
                int fp_is_void = 0;
                if (parser->current.type == TOKEN_VOID) {
                    int saved_pos = parser->lexer->pos;
                    Token saved_tok = parser->current;
                    parser->current = lexer_next_token(parser->lexer);
                    if (parser->current.type == TOKEN_RPAREN) {
                        fp_is_void = 1; /* true (void) — zero parameters */
                    } else {
                        parser->lexer->pos = saved_pos;
                        parser->current = saved_tok;
                    }
                }
                if (!fp_is_void && parser->current.type != TOKEN_RPAREN) {
                    while (1) {
                        if (count >= FUNC_TYPE_MAX_PARAMS) {
                            PARSER_ERROR(parser,
                                    "error: too many parameters in function pointer"
                                    " type (max %d)\n", FUNC_TYPE_MAX_PARAMS);
                        }
                        /* Stage 39: consume optional const qualifier.
                         * Stage 82-04: also consume optional volatile qualifier.
                         * Stage 106: also consume optional restrict qualifier. */
                        while (parser->current.type == TOKEN_CONST   ||
                               parser->current.type == TOKEN_VOLATILE ||
                               parser->current.type == TOKEN_RESTRICT)
                            parser->current = lexer_next_token(parser->lexer);
                        Type *pt = parse_type_specifier(parser, NULL);
                        /* Stage 159: trailing qualifiers after base type. */
                        while (parser->current.type == TOKEN_CONST   ||
                               parser->current.type == TOKEN_VOLATILE ||
                               parser->current.type == TOKEN_RESTRICT)
                            parser->current = lexer_next_token(parser->lexer);
                        int stars = 0;
                        while (parser->current.type == TOKEN_STAR) {
                            stars++;
                            parser->current = lexer_next_token(parser->lexer);
                        }
                        /* Optional parameter name — consume and discard. */
                        if (parser->current.type == TOKEN_IDENTIFIER)
                            parser->current = lexer_next_token(parser->lexer);
                        /* C99 §6.7.5.3p7: array param adjusted to pointer (e.g. char *argv[]). */
                        while (parser->current.type == TOKEN_LBRACKET) {
                            parser->current = lexer_next_token(parser->lexer);
                            while (parser->current.type != TOKEN_RBRACKET &&
                                   parser->current.type != TOKEN_EOF)
                                parser->current = lexer_next_token(parser->lexer);
                            if (parser->current.type == TOKEN_RBRACKET)
                                parser->current = lexer_next_token(parser->lexer);
                            stars++;
                        }
                        for (int i = 0; i < stars; i++)
                            pt = type_pointer(pt);
                        d.fp_param_types[count++] = pt;
                        if (parser->current.type != TOKEN_COMMA) break;
                        parser->current = lexer_next_token(parser->lexer);
                    }
                }
                parser_expect(parser, TOKEN_RPAREN);
                d.fp_param_count = count;
                return d;
            } else {
                /* int (name)(...) — redundant-paren function declaration */
                d.is_function   = 1;
                d.pointer_count = outer_stars;
                return d;
            }
        }
        /* Plain parenthesized declarator: outer+inner stars are the pointer depth. */
        d.pointer_count = outer_stars + inner_stars;
        return d;
    }

    /* Non-parenthesized declarator: stars before the identifier. */
    d.pointer_count = outer_stars;
    Token name = parser_expect(parser, TOKEN_IDENTIFIER);
    d.name = name.value;
    if (parser->current.type == TOKEN_LBRACKET) {
        d.is_array = 1;
        /* Stage 86: parse first dimension (may be empty for initializer inference). */
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type != TOKEN_RBRACKET) {
            long length = eval_const_expr(parser, "array dimension");
            if (length <= 0) {
                PARSER_ERROR(parser, "error: array size must be greater than zero\n");
            }
            d.array_dims[d.array_dim_count++] = (int)length;
            d.array_length = (int)length;
            d.has_size = 1;
        }
        parser_expect(parser, TOKEN_RBRACKET);
        /* Stage 86: parse additional dimensions [N2][N3]... — all must be explicit. */
        while (parser->current.type == TOKEN_LBRACKET) {
            if (d.array_dim_count >= MAX_ARRAY_DIMS) {
                PARSER_ERROR(parser,
                        "error: too many array dimensions (max %d)\n", MAX_ARRAY_DIMS);
            }
            parser->current = lexer_next_token(parser->lexer);
            long length = eval_const_expr(parser, "array dimension");
            if (length <= 0) {
                PARSER_ERROR(parser, "error: array size must be greater than zero\n");
            }
            d.array_dims[d.array_dim_count++] = (int)length;
            parser_expect(parser, TOKEN_RBRACKET);
        }
    } else if (parser->current.type == TOKEN_LPAREN) {
        d.is_function = 1;
    }
    return d;
}

/*
 * Stage 26: build the function-pointer Type* from a ParsedDeclarator that
 * has is_func_pointer set.  The parameter list was already consumed inside
 * parse_declarator and stored in d->fp_param_types / d->fp_param_count.
 *
 *   base_type + fp_outer_stars  →  return type
 *   type_function(return_type, params)  →  function type
 *   wrap fp_inner_stars times in type_pointer  →  final type
 */
static Type *build_fp_type(Type *base_type, const ParsedDeclarator *d) {
    Type *ret = base_type;
    for (int i = 0; i < d->fp_outer_stars; i++)
        ret = type_pointer(ret);
    Type *ft = type_function(ret, d->fp_param_count,
                             (Type **)d->fp_param_types);
    for (int i = 0; i < d->fp_inner_stars; i++)
        ft = type_pointer(ft);
    return ft;
}

/*
 * <primary> ::= <int_literal> | "(" <expression> ")"
 */
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_assignment_expression(Parser *parser);
static ASTNode *parse_initializer(Parser *parser);

static ASTNode *parse_primary(Parser *parser) {
    if (parser->current.type == TOKEN_INT_LITERAL) {
        Token token = parser_expect(parser, TOKEN_INT_LITERAL);
        ASTNode *node = parser_node(parser, AST_INT_LITERAL, token.value);
        node->decl_type = token.literal_type;
        node->is_unsigned = token.is_unsigned;
        return node;
    }
    if (parser->current.type == TOKEN_FLOAT_LITERAL ||
        parser->current.type == TOKEN_DOUBLE_LITERAL) {
        Token ftoken = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *node = parser_node(parser, AST_FLOAT_LITERAL, ftoken.value);
        node->decl_type = (ftoken.type == TOKEN_FLOAT_LITERAL) ? TYPE_FLOAT : TYPE_DOUBLE;
        node->full_type  = (ftoken.type == TOKEN_FLOAT_LITERAL) ? type_float() : type_double();
        return node;
    }
    /* Stage 14-02: a string literal is a primary expression whose
     * logical type is char[N+1], where N is the literal's byte
     * length and the trailing slot holds the implicit NUL. The
     * payload bytes are referenced via node->value (a pointer into
     * lexer-owned storage) and the length is preserved on full_type->length.
     *
     * Stage 14-05: the decoded payload may contain embedded NUL
     * bytes (from `\0`), so the count is stashed on byte_length for
     * downstream consumers (notably codegen, where full_type is
     * rewritten to `char *` during the array-to-pointer decay and
     * the length on full_type is no longer reachable).
     *
     * Stage 95-09: node->value is now a const char * into lexer
     * storage. For a single literal, point directly to token.value.
     * For adjacent (concatenated) literals, collect bytes in a StrBuf
     * and store the result via lexer_store_bytes. */
    if (parser->current.type == TOKEN_STRING_LITERAL) {
        Token token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *node = parser_node(parser, AST_STRING_LITERAL, NULL);
        int total_len = (int)token.value_len;
        /* Stage 89: consume any adjacent string literal tokens and
         * concatenate their decoded bytes into the same node. */
        if (parser->current.type == TOKEN_STRING_LITERAL) {
            StrBuf sb;
            strbuf_init(&sb);
            strbuf_append_n(&sb, token.value, token.value_len);
            while (parser->current.type == TOKEN_STRING_LITERAL) {
                Token next = parser->current;
                parser->current = lexer_next_token(parser->lexer);
                strbuf_append_n(&sb, next.value, next.value_len);
                total_len += (int)next.value_len;
            }
            node->value = lexer_store_bytes(parser->lexer, sb.data, (size_t)total_len);
            strbuf_free(&sb);
        } else {
            node->value = token.value;
        }
        node->byte_length = total_len;
        node->decl_type = TYPE_ARRAY;
        node->full_type = type_array(type_char(), total_len + 1);
        return node;
    }
    /* Stage 15-02: a character literal is a primary expression of type
     * int. The token already carries the decoded byte at value[0] and
     * the evaluated integer at long_value. The integer value used by
     * codegen is recovered as (unsigned char)node->value[0].
     *
     * Stage 95-09: node->value points directly to lexer-owned storage
     * (token.value is a 1-byte null-terminated string in the lexer pool). */
    if (parser->current.type == TOKEN_CHAR_LITERAL) {
        Token token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *node = parser_node(parser, AST_CHAR_LITERAL, NULL);
        node->value = token.value;
        node->byte_length = 1;
        node->decl_type = TYPE_INT;
        return node;
    }
    if (parser->current.type == TOKEN_IDENTIFIER) {
        /* C99 §6.4.2.2: __func__ predefined identifier — the name of the
         * lexically-enclosing function as a static const char array. */
        if (strcmp(parser->current.value, "__func__") == 0) {
            if (!parser->current_func_name)
                PARSER_ERROR(parser,
                    "error: '__func__' used outside of a function body\n");
            parser->current = lexer_next_token(parser->lexer);
            const char *fname = parser->current_func_name;
            int len = (int)strlen(fname);
            ASTNode *node = parser_node(parser, AST_STRING_LITERAL,
                                        lexer_store_bytes(parser->lexer,
                                                          fname, (size_t)len));
            node->byte_length = len;
            node->decl_type   = TYPE_ARRAY;
            node->full_type   = type_array(type_char(), len + 1);
            return node;
        }
        /* Stage 75-03: recognize __builtin_va_* intrinsics before general
         * identifier resolution.  Each form has fixed argument counts and
         * specific rules; all produce typed AST nodes. */
        if (strcmp(parser->current.value, "__builtin_va_start") == 0) {
            parser->current = lexer_next_token(parser->lexer);
            parser_expect(parser, TOKEN_LPAREN);
            if (!parser->current_func_is_variadic) {
                PARSER_ERROR(parser,
                        "error: __builtin_va_start used outside a variadic function\n");
            }
            ASTNode *node = parser_node(parser, AST_BUILTIN_VA_START, "__builtin_va_start");
            ast_add_child(node, parse_assignment_expression(parser));
            if (parser->current.type != TOKEN_COMMA) {
                PARSER_ERROR(parser,
                        "error: __builtin_va_start requires exactly 2 arguments\n");
            }
            parser->current = lexer_next_token(parser->lexer);
            ast_add_child(node, parse_assignment_expression(parser));
            parser_expect(parser, TOKEN_RPAREN);
            return node;
        }
        if (strcmp(parser->current.value, "__builtin_va_end") == 0) {
            parser->current = lexer_next_token(parser->lexer);
            parser_expect(parser, TOKEN_LPAREN);
            ASTNode *node = parser_node(parser, AST_BUILTIN_VA_END, "__builtin_va_end");
            ast_add_child(node, parse_assignment_expression(parser));
            parser_expect(parser, TOKEN_RPAREN);
            return node;
        }
        if (strcmp(parser->current.value, "__builtin_va_copy") == 0) {
            parser->current = lexer_next_token(parser->lexer);
            parser_expect(parser, TOKEN_LPAREN);
            ASTNode *node = parser_node(parser, AST_BUILTIN_VA_COPY, "__builtin_va_copy");
            ast_add_child(node, parse_assignment_expression(parser));
            parser_expect(parser, TOKEN_COMMA);
            ast_add_child(node, parse_assignment_expression(parser));
            parser_expect(parser, TOKEN_RPAREN);
            return node;
        }
        if (strcmp(parser->current.value, "__builtin_va_arg") == 0) {
            parser->current = lexer_next_token(parser->lexer);
            parser_expect(parser, TOKEN_LPAREN);
            ASTNode *node = parser_node(parser, AST_BUILTIN_VA_ARG, NULL);
            ast_add_child(node, parse_assignment_expression(parser));
            parser_expect(parser, TOKEN_COMMA);
            Type *arg_type = parse_type_name(parser);
            node->decl_type = arg_type->kind;
            node->full_type = arg_type;
            node->value = type_kind_name(arg_type->kind);
            parser_expect(parser, TOKEN_RPAREN);
            return node;
        }
        /* Stage 29: fold enum constants to integer literals before any other
         * identifier resolution. */
        for (size_t i = 0; i < parser->enum_consts.len; i++) {
            EnumConst *ec = (EnumConst *)vec_get(&parser->enum_consts, i);
            if (strcmp(ec->name, parser->current.value) == 0) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%ld", ec->value);
                parser->current = lexer_next_token(parser->lexer);
                ASTNode *node = parser_node(parser, AST_INT_LITERAL,
                    lexer_store_bytes(parser->lexer, buf, strlen(buf)));
                node->decl_type = TYPE_INT;
                return node;
            }
        }
        Token token = parser_expect(parser, TOKEN_IDENTIFIER);
        if (parser->current.type == TOKEN_LPAREN) {
            parser_expect(parser, TOKEN_LPAREN);
            FuncSig *sig = parser_find_function(parser, token.value);
            if (sig) {
                /* Direct named function call */
                ASTNode *call = parser_node(parser, AST_FUNCTION_CALL, token.value);
                if (parser->current.type != TOKEN_RPAREN) {
                    ast_add_child(call, parse_assignment_expression(parser));
                    while (parser->current.type == TOKEN_COMMA) {
                        parser->current = lexer_next_token(parser->lexer);
                        ast_add_child(call, parse_assignment_expression(parser));
                    }
                }
                parser_expect(parser, TOKEN_RPAREN);
                /* Stage 57-03: variadic functions require at least the fixed
                 * parameter count; non-variadic functions require an exact match. */
                /* Stage 129: param_count == -1 means unknown arity (block-scope
                 * forward declaration); skip the argument count check.
                 * Stage 133: has_no_prototype means `int f()` — no parameter
                 * type information; any argument count is accepted. */
                if (sig->param_count != -1 && !sig->has_no_prototype) {
                    if (sig->is_variadic) {
                        if (call->child_count < sig->param_count) {
                            PARSER_ERROR(parser,
                                    "error: function '%s' expects at least %d arguments, got %d\n",
                                    token.value, sig->param_count, call->child_count);
                        }
                    } else if (sig->param_count != call->child_count) {
                        PARSER_ERROR(parser,
                                "error: function '%s' expects %d arguments, got %d\n",
                                token.value, sig->param_count, call->child_count);
                    }
                }
                /* The call expression is typed with the callee's declared
                 * return type so downstream type rules see it. */
                call->decl_type = sig->return_type;
                return call;
            } else {
                /* Stage 25-03: not a known function — treat as an indirect
                 * call through a function-pointer variable.  Semantic
                 * validation (callee type, argument count/types) is deferred
                 * to codegen. */
                ASTNode *callee = parser_node(parser, AST_VAR_REF, token.value);
                ASTNode *call = parser_node(parser, AST_INDIRECT_CALL, NULL);
                ast_add_child(call, callee);
                if (parser->current.type != TOKEN_RPAREN) {
                    ast_add_child(call, parse_assignment_expression(parser));
                    while (parser->current.type == TOKEN_COMMA) {
                        parser->current = lexer_next_token(parser->lexer);
                        ast_add_child(call, parse_assignment_expression(parser));
                    }
                }
                parser_expect(parser, TOKEN_RPAREN);
                return call;
            }
        }
        return parser_node(parser, AST_VAR_REF, token.value);
    }
    if (parser->current.type == TOKEN_LPAREN) {
        parser_expect(parser, TOKEN_LPAREN);
        ASTNode *expr = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        return expr;
    }
    PARSER_ERROR(parser, "error: expected expression, got '%s'\n",
            parser->current.value);
}

/*
 * <postfix_expression> ::= <primary_expression>
 *                          { "[" <expression> "]" | "++" | "--" }
 *
 * Stage 13-02 added the subscript suffix; Stage 13-05 formalizes the
 * pointer-base case so the same suffix applies whether the identifier
 * names an array or a pointer. The base must still be an identifier in
 * this stage. The wrapped node is AST_ARRAY_INDEX with children
 * [base, index]. Type-driven dispatch (array vs pointer) and the
 * integer-index check happen in codegen.
 */
/* Stage 98: forward declaration — defined after parse_initializer. */
static ASTNode *build_compound_literal(Parser *parser, Type *t);

/* Stage 98: apply any trailing postfix suffixes ([expr], .field, ->field,
 * (args), ++, --) to `base` and return the final expression node.
 * Extracted from parse_postfix so compound literals can also chain suffixes. */
static ASTNode *parse_postfix_suffixes(Parser *parser, ASTNode *expr) {
    while (parser->current.type == TOKEN_INCREMENT ||
           parser->current.type == TOKEN_DECREMENT ||
           parser->current.type == TOKEN_LBRACKET ||
           parser->current.type == TOKEN_LPAREN    ||
           parser->current.type == TOKEN_DOT       ||
           parser->current.type == TOKEN_ARROW) {
        /* Stage 25-03: call suffix — handles (*fp)(args) where the callee
         * expression is already parsed (e.g. as a grouped dereference). */
        if (parser->current.type == TOKEN_LPAREN) {
            parser->current = lexer_next_token(parser->lexer); /* consume "(" */
            ASTNode *call = parser_node(parser, AST_INDIRECT_CALL, NULL);
            ast_add_child(call, expr);
            if (parser->current.type != TOKEN_RPAREN) {
                ast_add_child(call, parse_assignment_expression(parser));
                while (parser->current.type == TOKEN_COMMA) {
                    parser->current = lexer_next_token(parser->lexer);
                    ast_add_child(call, parse_assignment_expression(parser));
                }
            }
            parser_expect(parser, TOKEN_RPAREN);
            expr = call;
            continue;
        }
        if (parser->current.type == TOKEN_LBRACKET) {
            /* Stage 28-04: also allow a parenthesized deref as the subscript
             * base, supporting (*ptr_to_array)[idx] patterns.
             * Stage 42: also allow a prior array subscript so that
             * pointer-array element subscripts like names[0][1] work.
             * Stage 78: also allow member/arrow access as subscript base so
             * that expr.field[i] and expr->field[i] chains work.
             * Stage 98: also allow compound literals as subscript base. */
            /* Stage 114: also allow string literals as subscript base
             * so "abc"[2] works (the literal decays to const char *). */
            if (expr->type != AST_VAR_REF && expr->type != AST_DEREF &&
                expr->type != AST_ARRAY_INDEX &&
                expr->type != AST_MEMBER_ACCESS &&
                expr->type != AST_ARROW_ACCESS &&
                expr->type != AST_COMPOUND_LITERAL &&
                expr->type != AST_STRING_LITERAL) {
                PARSER_ERROR(parser, "error: subscript base must be an identifier\n");
            }
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *index = parse_expression(parser);
            parser_expect(parser, TOKEN_RBRACKET);
            ASTNode *node = parser_node(parser, AST_ARRAY_INDEX, NULL);
            ast_add_child(node, expr);
            ast_add_child(node, index);
            expr = node;
            continue;
        }
        if (parser->current.type == TOKEN_DOT) {
            parser->current = lexer_next_token(parser->lexer);
            Token field = parser_expect(parser, TOKEN_IDENTIFIER);
            ASTNode *node = parser_node(parser, AST_MEMBER_ACCESS, field.value);
            ast_add_child(node, expr);
            expr = node;
            continue;
        }
        if (parser->current.type == TOKEN_ARROW) {
            parser->current = lexer_next_token(parser->lexer);
            Token field = parser_expect(parser, TOKEN_IDENTIFIER);
            ASTNode *node = parser_node(parser, AST_ARROW_ACCESS, field.value);
            ast_add_child(node, expr);
            expr = node;
            continue;
        }
        /* Stage 80: postfix ++/-- attaches to whatever postfix expression
         * has already been built (identifier, subscript, member, arrow, or
         * a chain thereof). Whether the operand is a modifiable lvalue is
         * enforced later during code generation. */
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *node = parser_node(parser, AST_POSTFIX_INC_DEC, op.value);
        ast_add_child(node, expr);
        expr = node;
    }
    return expr;
}

static ASTNode *parse_postfix(Parser *parser) {
    ASTNode *base;
    /* Stage 98: detect compound literals (type-name) { init } so that
     * constructs like &(T){...} and -(int){val} work — parse_cast handles
     * the case at the top of the expression tree; parse_postfix handles it
     * when reached via parse_unary (e.g. operand of a unary operator).
     * If (type-name) is not followed by { we restore and fall through to
     * parse_primary, which handles ordinary parenthesised expressions. */
    if (parser->current.type == TOKEN_LPAREN) {
        int saved_pos = parser->lexer->pos;
        Token saved_token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_CONST ||
            parser->current.type == TOKEN_VOLATILE ||
            parser->current.type == TOKEN_VOID ||
            parser->current.type == TOKEN_FLOAT ||
            parser->current.type == TOKEN_DOUBLE ||
            parser->current.type == TOKEN_BOOL ||
            parser->current.type == TOKEN_CHAR ||
            parser->current.type == TOKEN_SHORT ||
            parser->current.type == TOKEN_INT ||
            parser->current.type == TOKEN_LONG ||
            parser->current.type == TOKEN_SIGNED ||
            parser->current.type == TOKEN_UNSIGNED ||
            parser->current.type == TOKEN_STRUCT ||
            parser->current.type == TOKEN_UNION ||
            (parser->current.type == TOKEN_IDENTIFIER &&
             parser_find_typedef(parser, parser->current.value))) {
            Type *t = parse_type_name(parser);
            if (parser->current.type == TOKEN_RPAREN) {
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_LBRACE) {
                    base = build_compound_literal(parser, t);
                    return parse_postfix_suffixes(parser, base);
                }
            }
            /* Saw (type-name) but no { — not a compound literal.  Restore
             * the lexer to before the '(' and fall through to parse_primary
             * which handles parenthesised expressions. */
        }
        parser->lexer->pos = saved_pos;
        parser->current = saved_token;
    }
    base = parse_primary(parser);
    return parse_postfix_suffixes(parser, base);
}

/*
 * <unary_expr> ::= ( "+" | "-" | "!" | "~" | "++" | "--" | "*" | "&" ) <unary_expr>
 *                | <postfix_expression>
 *
 * Stage 12-02 adds unary "&" (address-of) and unary "*" (dereference).
 * Address-of requires an lvalue; the only lvalue this stage supports
 * is a plain variable reference, so the operand must be AST_VAR_REF.
 * Unary "*" only fires when "*" begins a unary expression — binary
 * "*" continues to work because parse_term consumes the left operand
 * before looking for "*".
 *
 * Stage 16-02 adds unary "~" (bitwise complement) on integer operands;
 * pointer/array operands are rejected at codegen.
 */
static ASTNode *parse_unary(Parser *parser) {
    if (parser->current.type == TOKEN_SIZEOF) {
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type != TOKEN_LPAREN) {
            /* <sizeof_expression> ::= "sizeof" <unary_expression> */
            ASTNode *operand = parse_unary(parser);
            ASTNode *node = parser_node(parser, AST_SIZEOF_EXPR, NULL);
            ast_add_child(node, operand);
            return node;
        }
        /* Peek past '(' to distinguish sizeof(type) from sizeof(expression) */
        parser->current = lexer_next_token(parser->lexer); /* consume '(' */
        if (parser->current.type == TOKEN_VOID ||
            parser->current.type == TOKEN_FLOAT ||
            parser->current.type == TOKEN_DOUBLE ||
            parser->current.type == TOKEN_CONST ||
            parser->current.type == TOKEN_VOLATILE ||
            parser->current.type == TOKEN_BOOL ||
            parser->current.type == TOKEN_CHAR ||
            parser->current.type == TOKEN_SHORT ||
            parser->current.type == TOKEN_INT ||
            parser->current.type == TOKEN_LONG ||
            parser->current.type == TOKEN_SIGNED ||
            parser->current.type == TOKEN_UNSIGNED ||
            parser->current.type == TOKEN_STRUCT ||
            parser->current.type == TOKEN_UNION ||
            (parser->current.type == TOKEN_IDENTIFIER &&
             parser_find_typedef(parser, parser->current.value))) {
            /* <sizeof_expression> ::= "sizeof" "(" <type_name> ")" */
            Type *t = parse_type_name(parser);
            if (t->kind == TYPE_VOID)
                PARSER_ERROR(parser, "error: sizeof applied to void type\n");
            if (t->kind == TYPE_ARRAY && t->length == 0)
                PARSER_ERROR(parser, "error: sizeof applied to incomplete array type\n");
            parser_expect(parser, TOKEN_RPAREN);
            ASTNode *node = parser_node(parser, AST_SIZEOF_TYPE, NULL);
            node->decl_type = t->kind;
            /* Stage 86: also store full_type for TYPE_ARRAY so codegen
             * can look up the total size of multi-dimensional array types. */
            if (t->kind == TYPE_STRUCT || t->kind == TYPE_UNION ||
                t->kind == TYPE_ARRAY)
                node->full_type = t;
            return node;
        }
        if (parser->current.type == TOKEN_RPAREN) {
            PARSER_ERROR(parser, "error: expected expression or type in sizeof\n");
        }
        /* sizeof(<expression>) */
        ASTNode *operand = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        ASTNode *node = parser_node(parser, AST_SIZEOF_EXPR, NULL);
        ast_add_child(node, operand);
        return node;
    }
    if (parser->current.type == TOKEN_INCREMENT ||
        parser->current.type == TOKEN_DECREMENT) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *operand = parse_unary(parser);
        /* Stage 80: prefix ++/-- accepts any unary expression as operand;
         * lvalue validity is enforced later during code generation. */
        ASTNode *node = parser_node(parser, AST_PREFIX_INC_DEC, op.value);
        ast_add_child(node, operand);
        return node;
    }
    if (parser->current.type == TOKEN_AMPERSAND) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *operand = parse_unary(parser);
        /* Stage 13-04: address-of also accepts an array subscript so
         * `&a[i]` produces a pointer to the i-th element.
         * Stage 91: member access (. and ->) are also addressable lvalues.
         * Stage 98: compound literals are addressable lvalues. */
        if (operand->type != AST_VAR_REF &&
            operand->type != AST_ARRAY_INDEX &&
            operand->type != AST_MEMBER_ACCESS &&
            operand->type != AST_ARROW_ACCESS &&
            operand->type != AST_COMPOUND_LITERAL) {
            PARSER_ERROR(parser, "error: address-of requires an lvalue\n");
        }
        ASTNode *node = parser_node(parser, AST_ADDR_OF, NULL);
        ast_add_child(node, operand);
        return node;
    }
    if (parser->current.type == TOKEN_STAR) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *operand = parse_unary(parser);
        ASTNode *node = parser_node(parser, AST_DEREF, NULL);
        ast_add_child(node, operand);
        return node;
    }
    if (parser->current.type == TOKEN_PLUS ||
        parser->current.type == TOKEN_MINUS ||
        parser->current.type == TOKEN_BANG ||
        parser->current.type == TOKEN_TILDE) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *operand = parse_unary(parser);
        ASTNode *unary = parser_node(parser, AST_UNARY_OP, op.value);
        ast_add_child(unary, operand);
        return unary;
    }
    return parse_postfix(parser);
}

/*
 * <cast_expression> ::= "(" <type_name> ")" <cast_expression>
 *                     | <unary_expression>
 *
 * When the current token is '(', save lexer state and peek past it to
 * decide between a cast and a parenthesized expression. If the next
 * token is an integer-type keyword, consume "(", <type_name>, ")" and
 * recurse to parse the operand cast expression. Otherwise restore
 * the saved state and fall through to parse_unary — parenthesized
 * expressions are then handled by parse_primary as before.
 */
/*
 * <cast_expression> ::= "(" <type_name> ")" "{" <initializer_list> [ "," ] "}"
 *                     | "(" <type_name> ")" <cast_expression>
 *                     | <unary_expression>
 *
 * Stage 98: after the closing ")" is consumed, check for "{" to detect a
 * compound literal. If found, delegate to build_compound_literal and apply
 * postfix suffixes. Otherwise fall through to the regular cast path.
 */
static ASTNode *parse_cast(Parser *parser) {
    if (parser->current.type == TOKEN_LPAREN) {
        int saved_pos = parser->lexer->pos;
        Token saved_token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_CONST ||
            parser->current.type == TOKEN_VOLATILE ||
            parser->current.type == TOKEN_VOID ||
            parser->current.type == TOKEN_FLOAT ||
            parser->current.type == TOKEN_DOUBLE ||
            parser->current.type == TOKEN_BOOL ||
            parser->current.type == TOKEN_CHAR ||
            parser->current.type == TOKEN_SHORT ||
            parser->current.type == TOKEN_INT ||
            parser->current.type == TOKEN_LONG ||
            parser->current.type == TOKEN_SIGNED ||
            parser->current.type == TOKEN_UNSIGNED ||
            parser->current.type == TOKEN_STRUCT ||
            parser->current.type == TOKEN_UNION ||
            (parser->current.type == TOKEN_IDENTIFIER &&
             parser_find_typedef(parser, parser->current.value))) {
            Type *cast_type = parse_type_name(parser);
            parser_expect(parser, TOKEN_RPAREN);
            if (parser->current.type == TOKEN_LBRACE) {
                /* Stage 98: compound literal — (type-name) { initializer-list } */
                ASTNode *lit = build_compound_literal(parser, cast_type);
                return parse_postfix_suffixes(parser, lit);
            }
            /* Regular cast: reject omitted array dimension. */
            if (cast_type->kind == TYPE_ARRAY && cast_type->length == 0)
                PARSER_ERROR(parser, "error: array type in cast has omitted size\n");
            ASTNode *operand = parse_cast(parser);
            ASTNode *cast = parser_node(parser, AST_CAST, NULL);
            cast->decl_type = cast_type->kind;
            cast->is_unsigned = !cast_type->is_signed;
            if (cast_type->kind == TYPE_POINTER) {
                cast->full_type = cast_type;
            }
            ast_add_child(cast, operand);
            return cast;
        }
        /* Not a cast — restore lexer state */
        parser->lexer->pos = saved_pos;
        parser->current = saved_token;
    }
    return parse_unary(parser);
}

/*
 * <term> ::= <cast_expression> { ("*" | "/" | "%") <cast_expression> }*
 */
static ASTNode *parse_term(Parser *parser) {
    ASTNode *left = parse_cast(parser);
    while (parser->current.type == TOKEN_STAR ||
           parser->current.type == TOKEN_SLASH ||
           parser->current.type == TOKEN_PERCENT) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_cast(parser);
        ASTNode *binop = parser_node(parser, AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <additive_expr> ::= <term> { ("+" | "-") <term> }*
 */
static ASTNode *parse_additive(Parser *parser) {
    ASTNode *left = parse_term(parser);
    while (parser->current.type == TOKEN_PLUS ||
           parser->current.type == TOKEN_MINUS) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_term(parser);
        ASTNode *binop = parser_node(parser, AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <shift_expr> ::= <additive_expr> { ("<<" | ">>") <additive_expr> }*
 *
 * Stage 16-03: shift expressions sit between additive and relational
 * so `1 + 2 << 3` parses as `(1 + 2) << 3` and `1 << 3 > 4` parses as
 * `(1 << 3) > 4`. Left-associative.
 */
static ASTNode *parse_shift(Parser *parser) {
    ASTNode *left = parse_additive(parser);
    while (parser->current.type == TOKEN_LEFT_SHIFT ||
           parser->current.type == TOKEN_RIGHT_SHIFT) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_additive(parser);
        ASTNode *binop = parser_node(parser, AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <relational_expr> ::= <shift_expr> { ("<" | "<=" | ">" | ">=") <shift_expr> }*
 */
static ASTNode *parse_relational(Parser *parser) {
    ASTNode *left = parse_shift(parser);
    while (parser->current.type == TOKEN_LT ||
           parser->current.type == TOKEN_LE ||
           parser->current.type == TOKEN_GT ||
           parser->current.type == TOKEN_GE) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_shift(parser);
        ASTNode *binop = parser_node(parser, AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <equality_expr> ::= <relational_expr> { ("==" | "!=") <relational_expr> }*
 */
static ASTNode *parse_equality(Parser *parser) {
    ASTNode *left = parse_relational(parser);
    while (parser->current.type == TOKEN_EQ ||
           parser->current.type == TOKEN_NE) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_relational(parser);
        ASTNode *binop = parser_node(parser, AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <bitwise_and_expression> ::= <equality_expression> { "&" <equality_expression> }
 *
 * Stage 16-04: bitwise AND on integer operands. The same `&` lexeme is
 * used by unary address-of in `parse_unary`; the parser reaches this
 * production only after a primary/postfix expression has been
 * consumed, so a `&` seen here is unambiguously the binary form.
 */
static ASTNode *parse_bitwise_and(Parser *parser) {
    ASTNode *left = parse_equality(parser);
    while (parser->current.type == TOKEN_AMPERSAND) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_equality(parser);
        ASTNode *binop = parser_node(parser, AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <bitwise_xor_expression> ::= <bitwise_and_expression> { "^" <bitwise_and_expression> }
 */
static ASTNode *parse_bitwise_xor(Parser *parser) {
    ASTNode *left = parse_bitwise_and(parser);
    while (parser->current.type == TOKEN_CARET) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_bitwise_and(parser);
        ASTNode *binop = parser_node(parser, AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <bitwise_or_expression> ::= <bitwise_xor_expression> { "|" <bitwise_xor_expression> }
 */
static ASTNode *parse_bitwise_or(Parser *parser) {
    ASTNode *left = parse_bitwise_xor(parser);
    while (parser->current.type == TOKEN_PIPE) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_bitwise_xor(parser);
        ASTNode *binop = parser_node(parser, AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <logical_and_expression> ::= <bitwise_or_expression> { "&&" <bitwise_or_expression> }
 */
static ASTNode *parse_logical_and(Parser *parser) {
    ASTNode *left = parse_bitwise_or(parser);
    while (parser->current.type == TOKEN_AND_AND) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_bitwise_or(parser);
        ASTNode *binop = parser_node(parser, AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <logical_or_expression> ::= <logical_and_expression> { "||" <logical_and_expression> }
 */
static ASTNode *parse_logical_or(Parser *parser) {
    ASTNode *left = parse_logical_and(parser);
    while (parser->current.type == TOKEN_OR_OR) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_logical_and(parser);
        ASTNode *binop = parser_node(parser, AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <conditional_expression> ::= <logical_or_expression>
 *                            | <logical_or_expression> "?" <expression> ":" <conditional_expression>
 *
 * Stage 18: right-associative ternary. The true branch is a full
 * <expression> (allowing assignment); the false branch recurses into
 * <conditional_expression> for right-associativity.
 */
static ASTNode *parse_conditional(Parser *parser) {
    ASTNode *cond = parse_logical_or(parser);
    if (parser->current.type != TOKEN_QUESTION) {
        return cond;
    }
    parser->current = lexer_next_token(parser->lexer); /* consume '?' */
    if (parser->current.type == TOKEN_COLON) {
        PARSER_ERROR(parser, "error: expected expression after '?'\n");
    }
    ASTNode *true_expr = parse_expression(parser);
    if (parser->current.type != TOKEN_COLON) {
        PARSER_ERROR(parser, "error: expected ':' in conditional expression\n");
    }
    parser->current = lexer_next_token(parser->lexer); /* consume ':' */
    ASTNode *false_expr = parse_conditional(parser);
    ASTNode *node = parser_node(parser, AST_CONDITIONAL_EXPR, NULL);
    ast_add_child(node, cond);
    ast_add_child(node, true_expr);
    ast_add_child(node, false_expr);
    return node;
}

/*
 * Stage 32: <initializer> ::= <assignment_expression>
 *                            | "{" <initializer_list> [ "," ] "}"
 *
 * Stage 97: <initializer_list> elements may carry an optional designator:
 *   <initializer_element> ::= <designator_list> "=" <initializer>
 *                           | <initializer>
 *   <designator> ::= "." IDENTIFIER | "[" <constant_integer_expression> "]"
 *
 * Returns an AST_INITIALIZER_LIST node when a brace-initializer is
 * parsed; otherwise returns the result of parse_assignment_expression.
 */

/* Stage 97: parse one element of an initializer list. */
static ASTNode *parse_initializer_element(Parser *parser) {
    if (parser->current.type == TOKEN_DOT) {
        parser->current = lexer_next_token(parser->lexer);
        Token id = parser_expect(parser, TOKEN_IDENTIFIER);
        /* Reject chained designators before consuming '='. */
        if (parser->current.type == TOKEN_DOT ||
            parser->current.type == TOKEN_LBRACKET) {
            PARSER_ERROR(parser, "error: chained designators not yet supported\n");
        }
        parser_expect(parser, TOKEN_ASSIGN);
        ASTNode *node = parser_node(parser, AST_DESIGNATED_INIT, id.value);
        node->byte_length = 0;
        ast_add_child(node, parse_initializer(parser));
        return node;
    } else if (parser->current.type == TOKEN_LBRACKET) {
        parser->current = lexer_next_token(parser->lexer);
        long index = eval_const_expr(parser, "array designator index");
        if (index < 0) {
            PARSER_ERROR(parser,
                "error: array designator index must be non-negative\n");
        }
        parser_expect(parser, TOKEN_RBRACKET);
        /* Reject chained designators before consuming '='. */
        if (parser->current.type == TOKEN_DOT ||
            parser->current.type == TOKEN_LBRACKET) {
            PARSER_ERROR(parser, "error: chained designators not yet supported\n");
        }
        parser_expect(parser, TOKEN_ASSIGN);
        ASTNode *node = parser_node(parser, AST_DESIGNATED_INIT, NULL);
        node->byte_length = (int)index;
        ast_add_child(node, parse_initializer(parser));
        return node;
    } else {
        return parse_initializer(parser);
    }
}

static ASTNode *parse_initializer(Parser *parser) {
    if (parser->current.type != TOKEN_LBRACE)
        return parse_assignment_expression(parser);

    /* Consume "{". */
    parser->current = lexer_next_token(parser->lexer);

    ASTNode *list = parser_node(parser, AST_INITIALIZER_LIST, NULL);

    /* Empty brace-initializer "{}" — zero-fill everything. */
    if (parser->current.type == TOKEN_RBRACE) {
        parser->current = lexer_next_token(parser->lexer);
        return list;
    }

    ast_add_child(list, parse_initializer_element(parser));
    while (parser->current.type == TOKEN_COMMA) {
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_RBRACE)
            break; /* trailing comma */
        ast_add_child(list, parse_initializer_element(parser));
    }
    if (parser->current.type != TOKEN_RBRACE) {
        PARSER_ERROR(parser, "error: expected '}' to close initializer list\n");
    }
    parser->current = lexer_next_token(parser->lexer);
    return list;
}

/* Stage 98: infer array length from a compound literal's initializer list.
 * Simulates the element cursor: for each child, if it is an array-index
 * designator ([N]) the cursor jumps to N; otherwise the cursor advances by 1.
 * Returns the number of elements implied (= max cursor index reached + 1). */
static int infer_compound_literal_array_length(ASTNode *list) {
    int cur = 0;
    int max_idx = 0;
    int i;
    for (i = 0; i < list->child_count; i++) {
        ASTNode *child = list->children[i];
        if (child->type == AST_DESIGNATED_INIT && child->value == NULL) {
            cur = child->byte_length;
        }
        if (cur >= max_idx)
            max_idx = cur;
        cur++;
    }
    return (list->child_count == 0) ? 0 : max_idx + 1;
}

/* Stage 98: build_compound_literal — called from parse_cast when (type-name)
 * is followed by "{".  Current token is "{" on entry. */
static ASTNode *build_compound_literal(Parser *parser, Type *t) {
    /* Reject unsupported types. */
    if (t->kind == TYPE_VOID || t->kind == TYPE_FUNCTION) {
        PARSER_ERROR(parser, "error: invalid type for compound literal\n");
    }

    /* Build a type-label string for print_ast output. */
    char label_buf[128];
    if (t->kind == TYPE_STRUCT) {
        const char *tname = NULL;
        size_t ti;
        for (ti = 0; ti < parser->struct_tags.len; ti++) {
            StructTag *st = (StructTag *)vec_get(&parser->struct_tags, ti);
            if (st->type == t) { tname = st->tag; break; }
        }
        if (tname)
            snprintf(label_buf, sizeof(label_buf), "struct %s", tname);
        else
            snprintf(label_buf, sizeof(label_buf), "struct <anon>");
    } else if (t->kind == TYPE_UNION) {
        const char *tname = NULL;
        size_t ti;
        for (ti = 0; ti < parser->union_tags.len; ti++) {
            UnionTag *ut = (UnionTag *)vec_get(&parser->union_tags, ti);
            if (ut->type == t) { tname = ut->tag; break; }
        }
        if (tname)
            snprintf(label_buf, sizeof(label_buf), "union %s", tname);
        else
            snprintf(label_buf, sizeof(label_buf), "union <anon>");
    } else if (t->kind == TYPE_ARRAY) {
        const char *en = t->base ? type_kind_name(t->base->kind) : "?";
        if (t->length == 0)
            snprintf(label_buf, sizeof(label_buf), "%s[]", en);
        else
            snprintf(label_buf, sizeof(label_buf), "%s[%d]", en, t->length);
    } else if (t->kind == TYPE_POINTER) {
        const char *en = (t->base) ? type_kind_name(t->base->kind) : "?";
        snprintf(label_buf, sizeof(label_buf), "%s *", en);
    } else {
        const char *prefix = "";
        if (!t->is_signed &&
            t->kind != TYPE_BOOL &&
            t->kind != TYPE_UNSIGNED_LONG_LONG)
            prefix = "unsigned ";
        snprintf(label_buf, sizeof(label_buf), "%s%s", prefix, type_kind_name(t->kind));
    }

    ASTNode *node = parser_node(parser, AST_COMPOUND_LITERAL,
        lexer_store_bytes(parser->lexer, label_buf, strlen(label_buf)));
    node->decl_type = t->kind;
    node->byte_length = 0;  /* set by pre-scan in codegen */

    if (t->kind == TYPE_STRUCT || t->kind == TYPE_UNION) {
        node->full_type = t;
        /* parse_initializer consumes "{ initializer-list }" */
        ASTNode *list = parse_initializer(parser);
        ast_add_child(node, list);
    } else if (t->kind == TYPE_ARRAY) {
        /* parse_initializer consumes "{ initializer-list }" */
        ASTNode *list = parse_initializer(parser);
        if (t->length == 0) {
            /* Infer array length from the initializer. */
            int inferred = infer_compound_literal_array_length(list);
            if (inferred == 0) inferred = 1;  /* at least 1 element */
            t = type_array(t->base, inferred);
            /* Update the label to show the inferred length. */
            const char *en = t->base ? type_kind_name(t->base->kind) : "?";
            snprintf(label_buf, sizeof(label_buf), "%s[%d]", en, inferred);
            node->value =
                lexer_store_bytes(parser->lexer, label_buf, strlen(label_buf));
        }
        node->full_type = t;
        node->is_unsigned = (t->base && !t->base->is_signed &&
                             t->base->kind != TYPE_BOOL &&
                             t->base->kind != TYPE_UNSIGNED_LONG_LONG) ? 1 : 0;
        ast_add_child(node, list);
    } else {
        /* Scalar: parse "{ expr }" or bare "{ expr }". */
        node->full_type = (t->kind == TYPE_POINTER) ? t : NULL;
        node->is_unsigned = (!t->is_signed && t->kind != TYPE_BOOL &&
                             t->kind != TYPE_UNSIGNED_LONG_LONG) ? 1 : 0;
        /* consume { */
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *init = parse_assignment_expression(parser);
        if (parser->current.type == TOKEN_COMMA)
            parser->current = lexer_next_token(parser->lexer);
        parser_expect(parser, TOKEN_RBRACE);
        ast_add_child(node, init);
    }
    return node;
}

/*
 * <assignment_expression> ::= <identifier> <assignment_operator> <assignment_expression>
 *                            | <unary_expression> "=" <assignment_expression>
 *                            | <conditional_expression>
 *
 * Stage 12-03 adds the dereference-LHS form so `*p = value` parses.
 * Stage 18: non-assignment fallthrough calls parse_conditional instead
 * of parse_logical_or so `?:` expressions are recognized.
 * Stage 19: renamed from parse_expression; the new parse_expression adds
 * comma-operator handling above this level.
 */
static ASTNode *parse_assignment_expression(Parser *parser) {
    if (parser->current.type == TOKEN_IDENTIFIER) {
        int saved_pos = parser->lexer->pos;
        Token saved_token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_ASSIGN ||
            parser->current.type == TOKEN_PLUS_ASSIGN ||
            parser->current.type == TOKEN_MINUS_ASSIGN ||
            parser->current.type == TOKEN_STAR_ASSIGN ||
            parser->current.type == TOKEN_SLASH_ASSIGN ||
            parser->current.type == TOKEN_PERCENT_ASSIGN ||
            parser->current.type == TOKEN_LEFT_SHIFT_ASSIGN ||
            parser->current.type == TOKEN_RIGHT_SHIFT_ASSIGN ||
            parser->current.type == TOKEN_AMP_ASSIGN ||
            parser->current.type == TOKEN_CARET_ASSIGN ||
            parser->current.type == TOKEN_PIPE_ASSIGN) {
            Token op = parser->current;
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *rhs = parse_assignment_expression(parser);
            ASTNode *assign = parser_node(parser, AST_ASSIGNMENT, saved_token.value);
            if (op.type != TOKEN_ASSIGN) {
                /* a op= b  =>  a = a op b */
                ASTNode *var_ref = parser_node(parser, AST_VAR_REF, saved_token.value);
                const char *bin_op;
                switch (op.type) {
                case TOKEN_PLUS_ASSIGN:         bin_op = "+";  break;
                case TOKEN_MINUS_ASSIGN:        bin_op = "-";  break;
                case TOKEN_STAR_ASSIGN:         bin_op = "*";  break;
                case TOKEN_SLASH_ASSIGN:        bin_op = "/";  break;
                case TOKEN_PERCENT_ASSIGN:      bin_op = "%";  break;
                case TOKEN_LEFT_SHIFT_ASSIGN:   bin_op = "<<"; break;
                case TOKEN_RIGHT_SHIFT_ASSIGN:  bin_op = ">>"; break;
                case TOKEN_AMP_ASSIGN:          bin_op = "&";  break;
                case TOKEN_CARET_ASSIGN:        bin_op = "^";  break;
                default:                        bin_op = "|";  break;
                }
                ASTNode *binop = parser_node(parser, AST_BINARY_OP, bin_op);
                ast_add_child(binop, var_ref);
                ast_add_child(binop, rhs);
                ast_add_child(assign, binop);
            } else {
                ast_add_child(assign, rhs);
            }
            return assign;
        }
        /* Not assignment — restore lexer state */
        parser->lexer->pos = saved_pos;
        parser->current = saved_token;
    }
    ASTNode *lhs = parse_conditional(parser);
    if (parser->current.type == TOKEN_ASSIGN ||
        parser->current.type == TOKEN_PLUS_ASSIGN ||
        parser->current.type == TOKEN_MINUS_ASSIGN ||
        parser->current.type == TOKEN_STAR_ASSIGN ||
        parser->current.type == TOKEN_SLASH_ASSIGN ||
        parser->current.type == TOKEN_PERCENT_ASSIGN ||
        parser->current.type == TOKEN_LEFT_SHIFT_ASSIGN ||
        parser->current.type == TOKEN_RIGHT_SHIFT_ASSIGN ||
        parser->current.type == TOKEN_AMP_ASSIGN ||
        parser->current.type == TOKEN_CARET_ASSIGN ||
        parser->current.type == TOKEN_PIPE_ASSIGN) {
        if (lhs->type != AST_DEREF && lhs->type != AST_VAR_REF &&
            lhs->type != AST_ARRAY_INDEX &&
            lhs->type != AST_MEMBER_ACCESS &&
            lhs->type != AST_ARROW_ACCESS) {
            PARSER_ERROR(parser, "error: assignment target must be an lvalue\n");
        }
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *rhs = parse_assignment_expression(parser);
        ASTNode *assign = parser_node(parser, AST_ASSIGNMENT, NULL);
        ast_add_child(assign, lhs);
        if (op.type != TOKEN_ASSIGN) {
            /* Stage 79: general-lvalue compound assignment.
             * a op= b  =>  a = a op b. The lvalue is cloned so it can serve
             * both as the assignment target and the binary op's left operand. */
            const char *bin_op;
            switch (op.type) {
            case TOKEN_PLUS_ASSIGN:         bin_op = "+";  break;
            case TOKEN_MINUS_ASSIGN:        bin_op = "-";  break;
            case TOKEN_STAR_ASSIGN:         bin_op = "*";  break;
            case TOKEN_SLASH_ASSIGN:        bin_op = "/";  break;
            case TOKEN_PERCENT_ASSIGN:      bin_op = "%";  break;
            case TOKEN_LEFT_SHIFT_ASSIGN:   bin_op = "<<"; break;
            case TOKEN_RIGHT_SHIFT_ASSIGN:  bin_op = ">>"; break;
            case TOKEN_AMP_ASSIGN:          bin_op = "&";  break;
            case TOKEN_CARET_ASSIGN:        bin_op = "^";  break;
            default:                        bin_op = "|";  break;
            }
            ASTNode *binop = parser_node(parser, AST_BINARY_OP, bin_op);
            ast_add_child(binop, ast_clone(lhs));
            ast_add_child(binop, rhs);
            ast_add_child(assign, binop);
        } else {
            ast_add_child(assign, rhs);
        }
        return assign;
    }
    return lhs;
}

/*
 * <expression> ::= <assignment_expression> { "," <assignment_expression> }
 *
 * Stage 19: the comma operator has the lowest precedence among expression
 * operators. It is left-associative: `a, b, c` parses as `(a, b), c`.
 * The left operand is evaluated and its value discarded; the result is the
 * right operand's value and type. Commas in function-call argument lists
 * are separators, not comma operators — those call parse_assignment_expression
 * directly.
 */
static ASTNode *parse_expression(Parser *parser) {
    ASTNode *left = parse_assignment_expression(parser);
    while (parser->current.type == TOKEN_COMMA) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_assignment_expression(parser);
        ASTNode *comma = parser_node(parser, AST_COMMA_EXPR, ",");
        ast_add_child(comma, left);
        ast_add_child(comma, right);
        left = comma;
    }
    return left;
}

/*
 * <block> ::= "{" { <statement> } "}"
 */
static ASTNode *parse_statement(Parser *parser);

static ASTNode *parse_block(Parser *parser) {
    parser_expect(parser, TOKEN_LBRACE);
    ASTNode *block = parser_node(parser, AST_BLOCK, NULL);
    parser->scope_depth++;
    while (parser->current.type != TOKEN_RBRACE) {
        ast_add_child(block, parse_statement(parser));
    }
    parser_leave_scope(parser);
    parser_expect(parser, TOKEN_RBRACE);
    return block;
}

/*
 * <if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]
 */
static ASTNode *parse_if_statement(Parser *parser) {
    parser_expect(parser, TOKEN_IF);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    ASTNode *then_stmt = parse_statement(parser);

    ASTNode *if_node = parser_node(parser, AST_IF_STATEMENT, NULL);
    ast_add_child(if_node, condition);
    ast_add_child(if_node, then_stmt);

    if (parser->current.type == TOKEN_ELSE) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *else_stmt = parse_statement(parser);
        ast_add_child(if_node, else_stmt);
    }

    return if_node;
}

/*
 * <while_statement> ::= "while" "(" <expression> ")" <statement>
 */
static ASTNode *parse_while_statement(Parser *parser) {
    parser_expect(parser, TOKEN_WHILE);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser->loop_depth++;
    ASTNode *body = parse_statement(parser);
    parser->loop_depth--;

    ASTNode *while_node = parser_node(parser, AST_WHILE_STATEMENT, NULL);
    ast_add_child(while_node, condition);
    ast_add_child(while_node, body);

    return while_node;
}

/*
 * <do_while_statement> ::= "do" <statement> "while" "(" <expression> ")" ";"
 */
static ASTNode *parse_do_while_statement(Parser *parser) {
    parser_expect(parser, TOKEN_DO);
    parser->loop_depth++;
    ASTNode *body = parse_statement(parser);
    parser->loop_depth--;
    parser_expect(parser, TOKEN_WHILE);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_SEMICOLON);

    ASTNode *do_while_node = parser_node(parser, AST_DO_WHILE_STATEMENT, NULL);
    ast_add_child(do_while_node, body);
    ast_add_child(do_while_node, condition);

    return do_while_node;
}

/*
 * <for_statement> ::= "for" "(" <for_init> [<expression>] ";" [<expression>] ")" <statement>
 * <for_init>      ::= <declaration> | [<expression>] ";"
 *
 * Children layout: [init, condition, update, body]
 * Any of init/condition/update may be NULL when omitted.
 * init may be an AST_DECLARATION or AST_DECL_LIST (stage 76) when a C99
 * declaration appears in the for-init clause.  In that case the variable's
 * scope covers the condition, update, and body of the loop.
 */
static ASTNode *parse_for_statement(Parser *parser) {
    parser_expect(parser, TOKEN_FOR);
    parser_expect(parser, TOKEN_LPAREN);

    ASTNode *for_node = parser_node(parser, AST_FOR_STATEMENT, NULL);

    /* Open a new scope so any declaration in the init lives exactly as long
     * as the for statement (init + condition + update + body). */
    parser->scope_depth++;

    /* init: may be a declaration or an optional expression */
    ASTNode *init = NULL;
    int init_is_decl = 0;
    if (parser->current.type == TOKEN_CONST ||
        parser->current.type == TOKEN_VOLATILE ||
        parser->current.type == TOKEN_VOID  ||
        parser->current.type == TOKEN_FLOAT ||
        parser->current.type == TOKEN_DOUBLE ||
        parser->current.type == TOKEN_BOOL  ||
        parser->current.type == TOKEN_CHAR  ||
        parser->current.type == TOKEN_SHORT ||
        parser->current.type == TOKEN_INT   ||
        parser->current.type == TOKEN_LONG  ||
        parser->current.type == TOKEN_SIGNED ||
        parser->current.type == TOKEN_UNSIGNED ||
        parser->current.type == TOKEN_ENUM  ||
        parser->current.type == TOKEN_STRUCT ||
        parser->current.type == TOKEN_UNION  ||
        (parser->current.type == TOKEN_IDENTIFIER &&
         parser_find_typedef(parser, parser->current.value))) {
        /* parse_statement handles the full declaration (type specifier,
         * declarator list, optional initializer) and consumes the ";". */
        init = parse_statement(parser);
        init_is_decl = 1;
    } else if (parser->current.type != TOKEN_SEMICOLON) {
        init = parse_expression(parser);
    }
    if (!init_is_decl) {
        parser_expect(parser, TOKEN_SEMICOLON);
    }

    /* condition */
    ASTNode *condition = NULL;
    if (parser->current.type != TOKEN_SEMICOLON) {
        condition = parse_expression(parser);
    }
    parser_expect(parser, TOKEN_SEMICOLON);

    /* update */
    ASTNode *update = NULL;
    if (parser->current.type != TOKEN_RPAREN) {
        update = parse_expression(parser);
    }
    parser_expect(parser, TOKEN_RPAREN);

    parser->loop_depth++;
    ASTNode *body = parse_statement(parser);
    parser->loop_depth--;

    /* Close the for-scope: typedefs declared in the init are now invisible. */
    parser_leave_scope(parser);

    /* Store as children — NULLs are stored directly. Stage 92: children is
     * now a dynamically grown array, so append via ast_add_child rather than
     * writing fixed slots. The fixed four-slot layout (init, condition,
     * update, body) is preserved by the append order. */
    ast_add_child(for_node, init);
    ast_add_child(for_node, condition);
    ast_add_child(for_node, update);
    ast_add_child(for_node, body);

    return for_node;
}

/*
 * Stage 99–104: Compile-time integer constant expression evaluator.
 *
 * Grammar (loosest to tightest):
 *   const_expr         := const_conditional
 *   const_conditional  := const_logical_or ( '?' const_expr ':' const_conditional )?
 *   const_logical_or   := const_logical_and ( '||' const_logical_and )*
 *   const_logical_and  := const_bitwise_or  ( '&&' const_bitwise_or  )*
 *   const_bitwise_or   := const_bitwise_xor ( '|'  const_bitwise_xor )*
 *   const_bitwise_xor  := const_bitwise_and ( '^'  const_bitwise_and )*
 *   const_bitwise_and  := const_equality    ( '&'  const_equality    )*
 *   const_equality     := const_relational  ( ('==' | '!=') const_relational )*
 *   const_relational   := const_shift       ( ('<' | '<=' | '>' | '>=') const_shift )*
 *   const_shift        := const_additive    ( ('<<' | '>>') const_additive )*
 *   const_additive     := const_mult        ( ('+' | '-') const_mult )*
 *   const_mult         := const_unary       ( ('*' | '/' | '%') const_unary )*
 *   const_unary        := ('+' | '-' | '~' | '!') const_unary | const_primary
 *   const_primary      := INT_LITERAL | CHAR_LITERAL | enum-const-IDENTIFIER
 *                       | sizeof '(' type-name ')'
 *                       | '(' const_expr ')'
 *
 * context is "case label expression", "enumerator value",
 * "array designator index", or "file-scope initializer".
 * Calls PARSER_ERROR (does not return) if a non-constant operand is found.
 */
static long eval_const_primary(Parser *parser, const char *context) {
    if (parser->current.type == TOKEN_INT_LITERAL) {
        long v = parser->current.long_value;
        parser->current = lexer_next_token(parser->lexer);
        return v;
    }
    if (parser->current.type == TOKEN_CHAR_LITERAL) {
        long v = parser->current.long_value;
        parser->current = lexer_next_token(parser->lexer);
        return v;
    }
    if (parser->current.type == TOKEN_IDENTIFIER) {
        for (size_t i = 0; i < parser->enum_consts.len; i++) {
            EnumConst *ec = (EnumConst *)vec_get(&parser->enum_consts, i);
            if (strcmp(ec->name, parser->current.value) == 0) {
                long v = ec->value;
                parser->current = lexer_next_token(parser->lexer);
                return v;
            }
        }
        PARSER_ERROR(parser,
            "error: %s is not an integer constant expression\n", context);
    }
    if (parser->current.type == TOKEN_LPAREN) {
        parser->current = lexer_next_token(parser->lexer);
        /* Cast expression: '(' type-name ')' operand */
        int is_cast = (
            parser->current.type == TOKEN_VOID     ||
            parser->current.type == TOKEN_CHAR     ||
            parser->current.type == TOKEN_SHORT    ||
            parser->current.type == TOKEN_INT      ||
            parser->current.type == TOKEN_LONG     ||
            parser->current.type == TOKEN_SIGNED   ||
            parser->current.type == TOKEN_UNSIGNED ||
            parser->current.type == TOKEN_FLOAT    ||
            parser->current.type == TOKEN_DOUBLE   ||
            parser->current.type == TOKEN_CONST    ||
            parser->current.type == TOKEN_VOLATILE ||
            (parser->current.type == TOKEN_IDENTIFIER &&
             parser_find_typedef(parser, parser->current.value)));
        if (is_cast) {
            parse_type_name(parser); /* parse and discard the cast type (leaves ')' in stream) */
            parser_expect(parser, TOKEN_RPAREN); /* consume ')' of cast */
            return eval_const_unary(parser, context);
        }
        long v = eval_const_expr(parser, context);
        parser_expect(parser, TOKEN_RPAREN);
        return v;
    }
    if (parser->current.type == TOKEN_SIZEOF) {
        /* Stage 100: sizeof(type-name) in constant expression context. */
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type != TOKEN_LPAREN)
            PARSER_ERROR(parser,
                "error: sizeof requires a parenthesized type name in a constant expression\n");
        parser->current = lexer_next_token(parser->lexer); /* consume '(' */
        if (parser->current.type == TOKEN_VOID      ||
            parser->current.type == TOKEN_FLOAT     ||
            parser->current.type == TOKEN_DOUBLE    ||
            parser->current.type == TOKEN_CONST     ||
            parser->current.type == TOKEN_VOLATILE  ||
            parser->current.type == TOKEN_BOOL      ||
            parser->current.type == TOKEN_CHAR      ||
            parser->current.type == TOKEN_SHORT     ||
            parser->current.type == TOKEN_INT       ||
            parser->current.type == TOKEN_LONG      ||
            parser->current.type == TOKEN_SIGNED    ||
            parser->current.type == TOKEN_UNSIGNED  ||
            parser->current.type == TOKEN_STRUCT    ||
            parser->current.type == TOKEN_UNION     ||
            parser->current.type == TOKEN_ENUM      ||
            (parser->current.type == TOKEN_IDENTIFIER &&
             parser_find_typedef(parser, parser->current.value))) {
            Type *t = parse_type_name(parser);
            if (t->kind == TYPE_VOID)
                PARSER_ERROR(parser,
                    "error: sizeof applied to void type in constant expression\n");
            if (t->kind == TYPE_ARRAY && t->length == 0)
                PARSER_ERROR(parser,
                    "error: sizeof applied to incomplete array type in constant expression\n");
            parser_expect(parser, TOKEN_RPAREN);
            return (long)type_size(t);
        }
        /* sizeof(expr): parse the expression, derive size from the node's type. */
        {
            ASTNode *expr_node;
            Type *base_st;
            long sz;
            int k;
            int fi;
            sz = 4;
            k = TYPE_INT;
            fi = 0;
            base_st = NULL;
            expr_node = parse_expression(parser);
            if (expr_node != NULL && expr_node->full_type != NULL) {
                sz = (long)type_size(expr_node->full_type);
            } else if (expr_node != NULL &&
                       expr_node->type == AST_ARROW_ACCESS &&
                       expr_node->child_count > 0 &&
                       expr_node->children[0]->full_type != NULL &&
                       expr_node->children[0]->full_type->kind == TYPE_POINTER &&
                       expr_node->children[0]->full_type->base != NULL) {
                base_st = expr_node->children[0]->full_type->base;
                for (fi = 0; fi < base_st->field_count; fi++) {
                    if (strcmp(base_st->fields[fi].name, expr_node->value) == 0) {
                        if (base_st->fields[fi].full_type != NULL) {
                            sz = (long)base_st->fields[fi].full_type->size;
                        } else {
                            k = (int)base_st->fields[fi].kind;
                            if (k == TYPE_POINTER || k == TYPE_LONG ||
                                k == TYPE_LONG_LONG || k == TYPE_UNSIGNED_LONG_LONG ||
                                k == TYPE_DOUBLE)
                                sz = 8;
                            else if (k == TYPE_SHORT)
                                sz = 2;
                            else if (k == TYPE_CHAR || k == TYPE_BOOL)
                                sz = 1;
                            else
                                sz = 4;
                        }
                        break;
                    }
                }
            } else {
                k = (expr_node != NULL) ? (int)expr_node->decl_type : TYPE_INT;
                if (k == TYPE_POINTER || k == TYPE_LONG || k == TYPE_LONG_LONG ||
                    k == TYPE_UNSIGNED_LONG_LONG || k == TYPE_DOUBLE)
                    sz = 8;
                else if (k == TYPE_SHORT)
                    sz = 2;
                else if (k == TYPE_CHAR || k == TYPE_BOOL)
                    sz = 1;
                else
                    sz = 4;
            }
            if (expr_node != NULL) ast_free(expr_node);
            parser_expect(parser, TOKEN_RPAREN);
            return sz;
        }
    }
    PARSER_ERROR(parser,
        "error: %s is not an integer constant expression\n", context);
}

static long eval_const_unary(Parser *parser, const char *context) {
    if (parser->current.type == TOKEN_MINUS) {
        parser->current = lexer_next_token(parser->lexer);
        return -eval_const_unary(parser, context);
    }
    if (parser->current.type == TOKEN_PLUS) {
        parser->current = lexer_next_token(parser->lexer);
        return eval_const_unary(parser, context);
    }
    if (parser->current.type == TOKEN_TILDE) {
        parser->current = lexer_next_token(parser->lexer);
        return ~eval_const_unary(parser, context);
    }
    if (parser->current.type == TOKEN_BANG) {
        parser->current = lexer_next_token(parser->lexer);
        return !eval_const_unary(parser, context);
    }
    return eval_const_primary(parser, context);
}

static long eval_const_multiplicative(Parser *parser, const char *context) {
    long val = eval_const_unary(parser, context);
    while (parser->current.type == TOKEN_STAR ||
           parser->current.type == TOKEN_SLASH ||
           parser->current.type == TOKEN_PERCENT) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_unary(parser, context);
        if (op == TOKEN_STAR) {
            val *= rhs;
        } else {
            if (rhs == 0)
                PARSER_ERROR(parser,
                    "error: division by zero in constant expression\n");
            if (op == TOKEN_SLASH) val /= rhs;
            else                   val %= rhs;
        }
    }
    return val;
}

static long eval_const_additive(Parser *parser, const char *context) {
    long val = eval_const_multiplicative(parser, context);
    while (parser->current.type == TOKEN_PLUS ||
           parser->current.type == TOKEN_MINUS) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_multiplicative(parser, context);
        if (op == TOKEN_PLUS) val += rhs;
        else                  val -= rhs;
    }
    return val;
}

static long eval_const_shift(Parser *parser, const char *context) {
    long val = eval_const_additive(parser, context);
    while (parser->current.type == TOKEN_LEFT_SHIFT ||
           parser->current.type == TOKEN_RIGHT_SHIFT) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_additive(parser, context);
        if (op == TOKEN_LEFT_SHIFT) val <<= rhs;
        else                        val >>= rhs;
    }
    return val;
}

static long eval_const_relational(Parser *parser, const char *context) {
    long val = eval_const_shift(parser, context);
    while (parser->current.type == TOKEN_LT ||
           parser->current.type == TOKEN_LE ||
           parser->current.type == TOKEN_GT ||
           parser->current.type == TOKEN_GE) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_shift(parser, context);
        if      (op == TOKEN_LT) val = val <  rhs;
        else if (op == TOKEN_LE) val = val <= rhs;
        else if (op == TOKEN_GT) val = val >  rhs;
        else                     val = val >= rhs;
    }
    return val;
}

static long eval_const_equality(Parser *parser, const char *context) {
    long val = eval_const_relational(parser, context);
    while (parser->current.type == TOKEN_EQ ||
           parser->current.type == TOKEN_NE) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_relational(parser, context);
        if (op == TOKEN_EQ) val = val == rhs;
        else                val = val != rhs;
    }
    return val;
}

static long eval_const_bitwise_and(Parser *parser, const char *context) {
    long val = eval_const_equality(parser, context);
    while (parser->current.type == TOKEN_AMPERSAND) {
        parser->current = lexer_next_token(parser->lexer);
        val &= eval_const_equality(parser, context);
    }
    return val;
}

static long eval_const_bitwise_xor(Parser *parser, const char *context) {
    long val = eval_const_bitwise_and(parser, context);
    while (parser->current.type == TOKEN_CARET) {
        parser->current = lexer_next_token(parser->lexer);
        val ^= eval_const_bitwise_and(parser, context);
    }
    return val;
}

static long eval_const_bitwise_or(Parser *parser, const char *context) {
    long val = eval_const_bitwise_xor(parser, context);
    while (parser->current.type == TOKEN_PIPE) {
        parser->current = lexer_next_token(parser->lexer);
        val |= eval_const_bitwise_xor(parser, context);
    }
    return val;
}

static long eval_const_logical_and(Parser *parser, const char *context) {
    long val = eval_const_bitwise_or(parser, context);
    while (parser->current.type == TOKEN_AND_AND) {
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_bitwise_or(parser, context);
        val = (val && rhs) ? 1 : 0;
    }
    return val;
}

static long eval_const_logical_or(Parser *parser, const char *context) {
    long val = eval_const_logical_and(parser, context);
    while (parser->current.type == TOKEN_OR_OR) {
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_logical_and(parser, context);
        val = (val || rhs) ? 1 : 0;
    }
    return val;
}

static long eval_const_conditional(Parser *parser, const char *context) {
    long cond = eval_const_logical_or(parser, context);
    if (parser->current.type != TOKEN_QUESTION)
        return cond;
    parser->current = lexer_next_token(parser->lexer); /* consume '?' */
    long true_val  = eval_const_expr(parser, context);
    parser_expect(parser, TOKEN_COLON);
    long false_val = eval_const_conditional(parser, context);
    return cond ? true_val : false_val;
}

static long eval_const_expr(Parser *parser, const char *context) {
    return eval_const_conditional(parser, context);
}

/* Stage 128: compile-time floating-point constant expression evaluator.
 * Supports: FP/int literals, unary +/-, binary +/-/* and /, parentheses.
 * Returns a double; caller converts to float if needed. */
static double eval_fp_const_expr(Parser *parser, const char *context);

static double eval_fp_primary(Parser *parser, const char *context) {
    if (parser->current.type == TOKEN_FLOAT_LITERAL ||
        parser->current.type == TOKEN_DOUBLE_LITERAL) {
        double v = strtod(parser->current.value, NULL);
        parser->current = lexer_next_token(parser->lexer);
        return v;
    }
    if (parser->current.type == TOKEN_INT_LITERAL) {
        double v = (double)parser->current.long_value;
        parser->current = lexer_next_token(parser->lexer);
        return v;
    }
    if (parser->current.type == TOKEN_LPAREN) {
        parser->current = lexer_next_token(parser->lexer);
        double v = eval_fp_const_expr(parser, context);
        parser_expect(parser, TOKEN_RPAREN);
        return v;
    }
    PARSER_ERROR(parser,
        "error: %s requires a floating-point constant expression\n", context);
}

static double eval_fp_unary(Parser *parser, const char *context) {
    if (parser->current.type == TOKEN_MINUS) {
        parser->current = lexer_next_token(parser->lexer);
        return -eval_fp_unary(parser, context);
    }
    if (parser->current.type == TOKEN_PLUS) {
        parser->current = lexer_next_token(parser->lexer);
        return eval_fp_unary(parser, context);
    }
    return eval_fp_primary(parser, context);
}

static double eval_fp_mult(Parser *parser, const char *context) {
    double v = eval_fp_unary(parser, context);
    while (parser->current.type == TOKEN_STAR ||
           parser->current.type == TOKEN_SLASH) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        double r = eval_fp_unary(parser, context);
        v = (op == TOKEN_STAR) ? v * r : v / r;
    }
    return v;
}

static double eval_fp_const_expr(Parser *parser, const char *context) {
    double v = eval_fp_mult(parser, context);
    while (parser->current.type == TOKEN_PLUS ||
           parser->current.type == TOKEN_MINUS) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        double r = eval_fp_mult(parser, context);
        v = (op == TOKEN_PLUS) ? v + r : v - r;
    }
    return v;
}

/*
 * <switch_statement>   ::= "switch" "(" <expression> ")" <statement>
 * <labeled_statement>  ::= "case" <case_constant_expr> ":" <statement>
 *                        | "default" ":" <statement>
 *
 * Stage 10-03-03: the switch body is any statement. `case` and
 * `default` labels are parsed as labeled statements inside
 * parse_statement and are only legal while switch_depth > 0.
 * Stage 77: case expression is a compile-time constant (int/char
 * literal, enum constant, or simple +/- combination thereof).
 */
static ASTNode *parse_switch_statement(Parser *parser) {
    parser_expect(parser, TOKEN_SWITCH);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);

    ASTNode *switch_node = parser_node(parser, AST_SWITCH_STATEMENT, NULL);
    ast_add_child(switch_node, expr);

    parser->switch_depth++;
    ASTNode *body = parse_statement(parser);
    parser->switch_depth--;

    ast_add_child(switch_node, body);

    return switch_node;
}

/*
 * <statement> ::= <declaration>
 *               | <assignment_statement>
 *               | <return_statement>
 *               | <if_statement>
 *               | <while_statement>
 *               | <do_while_statement>
 *               | <for_statement>
 *               | <switch_statement>
 *               | <block>
 *               | <expression_stmt>
 */
static ASTNode *parse_statement(Parser *parser) {
    /* Stage 23: extern is not allowed at block scope. */
    if (parser->current.type == TOKEN_EXTERN) {
        PARSER_ERROR(parser,
                "error: storage class specifier not allowed in block scope\n");
    }
    /* Stage 138: auto at block scope is equivalent to default automatic storage. */
    if (parser->current.type == TOKEN_AUTO) {
        parser->current = lexer_next_token(parser->lexer);
        return parse_statement(parser);
    }
    /* Stage 138: register at block scope — allocate like automatic; forbid address-of. */
    if (parser->current.type == TOKEN_REGISTER) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *inner = parse_statement(parser);
        if (inner->type == AST_DECLARATION) {
            inner->storage_class = SC_REGISTER;
        } else if (inner->type == AST_DECL_LIST) {
            int i;
            for (i = 0; i < inner->child_count; i++) {
                if (inner->children[i]->type == AST_DECLARATION)
                    inner->children[i]->storage_class = SC_REGISTER;
            }
        }
        return inner;
    }
    /* Stage 71: static is allowed at block scope — consume the keyword,
     * parse the declaration, and mark all resulting AST_DECLARATION nodes
     * with SC_STATIC so codegen emits them to static storage. */
    if (parser->current.type == TOKEN_STATIC) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *inner = parse_statement(parser);
        if (inner->type == AST_DECLARATION) {
            inner->storage_class = SC_STATIC;
        } else if (inner->type == AST_DECL_LIST) {
            for (int i = 0; i < inner->child_count; i++) {
                if (inner->children[i]->type == AST_DECLARATION)
                    inner->children[i]->storage_class = SC_STATIC;
            }
        }
        return inner;
    }
    /* Stage 28-01/28-02/28-03/28-04: typedef declaration at block scope. */
    if (parser->current.type == TOKEN_TYPEDEF) {
        parser->current = lexer_next_token(parser->lexer);
        TypeKind base_kind;
        Type *base_type = parse_type_specifier(parser, &base_kind);
        ParsedDeclarator d = parse_declarator(parser);
        if (d.is_function) {
            /* Function-type typedef: 'typedef ret name(params);'
             * Skip the parameter list and register name as a long-sized type so
             * 'name *fp' is accepted as a function-pointer-sized field in structs. */
            {
                parser_expect(parser, TOKEN_LPAREN);
                int depth = 1;
                while (depth > 0 && parser->current.type != TOKEN_EOF) {
                    if      (parser->current.type == TOKEN_LPAREN) depth++;
                    else if (parser->current.type == TOKEN_RPAREN) depth--;
                    parser->current = lexer_next_token(parser->lexer);
                }
            }
            parser_expect(parser, TOKEN_SEMICOLON);
            parser_register_typedef(parser, d.name, TYPE_LONG, NULL);
            return parser_node(parser, AST_TYPEDEF_DECL, d.name);
        }
        /* Stage 28-04: array typedef — register with the full array Type*.
         * Stage 86: multi-dimensional array typedefs. */
        if (d.is_array) {
            if (!d.has_size) {
                PARSER_ERROR(parser, "error: array typedef requires explicit size\n");
            }
            parser_expect(parser, TOKEN_SEMICOLON);
            Type *array_type = build_array_type_from_dims(base_type,
                                                          d.array_dims, d.array_dim_count);
            parser_register_typedef(parser, d.name, TYPE_ARRAY, array_type);
            return parser_node(parser, AST_TYPEDEF_DECL, d.name);
        }
        if (parser->current.type == TOKEN_ASSIGN) {
            PARSER_ERROR(parser,
                    "error: typedef declaration cannot have an initializer\n");
        }
        parser_expect(parser, TOKEN_SEMICOLON);
        if (d.is_func_pointer) {
            Type *fp_type = build_fp_type(base_type, &d);
            parser_register_typedef(parser, d.name, TYPE_POINTER, fp_type);
            return parser_node(parser, AST_TYPEDEF_DECL, d.name);
        }
        Type *full_type = base_type;
        for (int i = 0; i < d.pointer_count; i++)
            full_type = type_pointer(full_type);
        TypeKind typedef_kind = (d.pointer_count > 0 || base_kind == TYPE_POINTER)
                                ? TYPE_POINTER : base_kind;
        /* Stage 40: for unsigned scalar typedefs, store the base Type* (is_signed=0)
         * as full_type so the signedness is preserved when the typedef is resolved. */
        Type *reg_full_type = (typedef_kind == TYPE_POINTER ||
                               typedef_kind == TYPE_STRUCT  ||
                               typedef_kind == TYPE_UNION)
                              ? full_type
                              : (!base_type->is_signed ? base_type : NULL);
        parser_register_typedef(parser, d.name, typedef_kind, reg_full_type);
        return parser_node(parser, AST_TYPEDEF_DECL, d.name);
    }
    /* Stage 159: GCC inline assembly — consume and discard the asm statement.
     * Handles:  __asm__("template" : output : input : clobbers);
     *           __asm__ volatile ("template" : ...);
     *           asm("template");
     * The entire parenthesised argument is skipped; the resulting AST node is
     * an empty block (a no-op) so codegen produces no instructions for it. */
    if (parser->current.type == TOKEN_IDENTIFIER &&
        (strcmp(parser->current.value, "__asm__") == 0 ||
         strcmp(parser->current.value, "__asm")   == 0 ||
         strcmp(parser->current.value, "asm")     == 0)) {
        parser->current = lexer_next_token(parser->lexer); /* consume keyword */
        /* optional 'volatile' qualifier */
        if (parser->current.type == TOKEN_VOLATILE)
            parser->current = lexer_next_token(parser->lexer);
        /* consume balanced '(...)' */
        if (parser->current.type == TOKEN_LPAREN) {
            int depth = 1;
            parser->current = lexer_next_token(parser->lexer);
            while (depth > 0 && parser->current.type != TOKEN_EOF) {
                if      (parser->current.type == TOKEN_LPAREN) depth++;
                else if (parser->current.type == TOKEN_RPAREN) depth--;
                if (depth > 0)
                    parser->current = lexer_next_token(parser->lexer);
            }
            if (parser->current.type == TOKEN_RPAREN)
                parser->current = lexer_next_token(parser->lexer);
        }
        parser_expect(parser, TOKEN_SEMICOLON);
        return parser_node(parser, AST_BLOCK, NULL); /* empty no-op */
    }
    /* labeled_statement: <identifier> ":" <statement> */
    if (parser->current.type == TOKEN_IDENTIFIER) {
        int saved_pos = parser->lexer->pos;
        Token saved_token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_COLON) {
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *node = parser_node(parser, AST_LABEL_STATEMENT, saved_token.value);
            ast_add_child(node, parse_statement(parser));
            return node;
        }
        /* Not a label — restore lexer state */
        parser->lexer->pos = saved_pos;
        parser->current = saved_token;
    }
    /* <declaration> ::= [<type_qualifier>] <type_specifier> <init_declarator> ";"
     * <init_declarator> ::= <declarator> [ "=" <initializer_expression> ]
     *
     * parse_type_specifier reads the base keyword; parse_declarator
     * reads the pointer stars, identifier, and optional array suffix.
     * The caller assembles the semantic Type from those two pieces.
     *
     * Stage 25-01: a function-pointer declarator (*fp)(params) allocates an
     * 8-byte local with decl_type=TYPE_POINTER.
     * Stage 25-03: optional initializer supported.
     * Stage 39: optional leading const qualifier.
     * Stage 82-04: optional leading volatile qualifier. */
    int local_is_const = 0;
    int local_is_volatile = 0;
    if (parser->current.type == TOKEN_CONST) {
        local_is_const = 1;
        parser->current = lexer_next_token(parser->lexer);
    } else if (parser->current.type == TOKEN_VOLATILE) {
        local_is_volatile = 1;
        parser->current = lexer_next_token(parser->lexer);
    }
    if (local_is_const || local_is_volatile ||
        parser->current.type == TOKEN_VOID ||
        parser->current.type == TOKEN_FLOAT ||
        parser->current.type == TOKEN_DOUBLE ||
        parser->current.type == TOKEN_BOOL ||
        parser->current.type == TOKEN_CHAR ||
        parser->current.type == TOKEN_SHORT ||
        parser->current.type == TOKEN_INT ||
        parser->current.type == TOKEN_LONG ||
        parser->current.type == TOKEN_SIGNED ||
        parser->current.type == TOKEN_UNSIGNED ||
        parser->current.type == TOKEN_ENUM ||
        parser->current.type == TOKEN_STRUCT ||
        parser->current.type == TOKEN_UNION ||
        (parser->current.type == TOKEN_IDENTIFIER &&
         parser_find_typedef(parser, parser->current.value))) {
        TypeKind base_kind;
        Type *base_type = parse_type_specifier(parser, &base_kind);

        /* Standalone type declaration with no variable (e.g. "struct S{};"). */
        if (parser->current.type == TOKEN_SEMICOLON) {
            parser_expect(parser, TOKEN_SEMICOLON);
            return parser_node(parser, AST_TYPEDEF_DECL, "");
        }

        ParsedDeclarator d = parse_declarator(parser);

        /* Stage 129: block-scope function declaration (C99 §6.2.1p4).
         * Consume the parameter list and semicolon; register the function
         * if not already known; return a no-op node. */
        if (d.is_function) {
            /* Consume '(' and balance until matching ')'. */
            parser_expect(parser, TOKEN_LPAREN);
            int depth = 1;
            while (depth > 0 && parser->current.type != TOKEN_EOF) {
                if (parser->current.type == TOKEN_LPAREN) depth++;
                else if (parser->current.type == TOKEN_RPAREN) {
                    depth--;
                    if (depth == 0) break;
                }
                parser->current = lexer_next_token(parser->lexer);
            }
            parser_expect(parser, TOKEN_RPAREN);
            parser_expect(parser, TOKEN_SEMICOLON);
            if (!parser_find_function(parser, d.name)) {
                parser_register_function(parser, d.name, -1, 0, base_kind,
                                         NULL, SC_NONE, 0, 0);
            }
            return parser_node(parser, AST_TYPEDEF_DECL, "");
        }

        /* Reject `void x;` — void cannot be an object type. */
        if (base_kind == TYPE_VOID && d.pointer_count == 0 &&
            !d.is_func_pointer && !d.is_array) {
            PARSER_ERROR(parser,
                    "error: cannot declare variable '%s' of type void\n", d.name);
        }

        if (d.is_func_pointer) {
            ASTNode *decl = parser_node(parser, AST_DECLARATION, d.name);
            decl->decl_type = TYPE_POINTER;
            decl->full_type = build_fp_type(base_type, &d);
            /* Stage 25-03: optional initializer — accepts any assignment
             * expression; codegen validates the type. */
            if (parser->current.type == TOKEN_ASSIGN) {
                parser->current = lexer_next_token(parser->lexer);
                ast_add_child(decl, parse_assignment_expression(parser));
            }
            parser_expect(parser, TOKEN_SEMICOLON);
            return decl;
        }

        /* Stage 66: when const precedes a pointer declaration (const T *p),
         * the base type is const-qualified; build a const copy so the
         * pointer type chain carries is_const on the pointee.
         * Stage 82-04: same for volatile. */
        Type *effective_base = base_type;
        if (local_is_const && d.pointer_count > 0)
            effective_base = type_const_copy(base_type);
        else if (local_is_volatile && d.pointer_count > 0)
            effective_base = type_volatile_copy(base_type);

        /* Build the fully-wrapped element type: base + pointer levels. */
        Type *full_type = effective_base;
        for (int i = 0; i < d.pointer_count; i++) {
            full_type = type_pointer(full_type);
        }

        ASTNode *decl = parser_node(parser, AST_DECLARATION, d.name);
        /* Stage 39: const applies to the variable when no pointer depth.
         * Stage 66: also applies when the pointer itself is const (T * const p). */
        decl->is_const = ((local_is_const && d.pointer_count == 0 && !d.is_array) ||
                          d.pointer_is_const) ? 1 : 0;

        /* Stage 13-01 / 14-06: optional array suffix on the declarator.
         * An omitted size is only valid with a string-literal initializer;
         * string initializers are only valid when the element type is char. */
        if (d.is_array) {
            int has_size = d.has_size;
            int length = d.array_length;

            ASTNode *init_node = NULL;
            if (parser->current.type == TOKEN_ASSIGN) {
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_LBRACE) {
                    /* Stage 32: brace-initializer list for array.
                     * Stage 43: allow omitted size — infer from element count. */
                    init_node = parse_initializer(parser);
                    if (!has_size) {
                        length = init_node->child_count;
                        has_size = 1;
                    }
                } else if (parser->current.type == TOKEN_STRING_LITERAL) {
                    if (full_type->kind != TYPE_CHAR) {
                        PARSER_ERROR(parser, 
                                "error: string initializer only supported for char arrays\n");
                    }
                    Token str_tok = parser->current;
                    parser->current = lexer_next_token(parser->lexer);
                    ASTNode *str_init = parser_node(parser, AST_STRING_LITERAL, NULL);
                    str_init->value = str_tok.value;
                    str_init->byte_length = (int)str_tok.value_len;
                    str_init->decl_type = TYPE_ARRAY;
                    str_init->full_type = type_array(type_char(), (int)str_tok.value_len + 1);
                    int needed = str_init->byte_length + 1;
                    if (has_size) {
                        if (length < needed) {
                            PARSER_ERROR(parser,
                                    "error: array too small for string literal initializer\n");
                        }
                    } else {
                        length = needed;
                    }
                    init_node = str_init;
                } else {
                    if (!has_size) {
                        PARSER_ERROR(parser, 
                                "error: omitted array size requires string literal initializer\n");
                    } else {
                        PARSER_ERROR(parser, 
                                "error: array initializer must be a brace-enclosed list or string literal\n");
                    }
                }
            } else if (!has_size) {
                PARSER_ERROR(parser, 
                        "error: array size required unless initialized from string literal\n");
            }

            /* Stage 86: build multi-dim type. For single-dim (or inferred size),
             * update array_dims[0] so build_array_type_from_dims gets the right length. */
            d.array_dims[0] = length;
            if (d.array_dim_count == 0) d.array_dim_count = 1;
            Type *array_type = build_array_type_from_dims(full_type,
                                                          d.array_dims, d.array_dim_count);
            decl->decl_type = TYPE_ARRAY;
            decl->full_type = array_type;
            if (init_node) {
                ast_add_child(decl, init_node);
            }
            parser_expect(parser, TOKEN_SEMICOLON);
            return decl;
        }
        if (d.pointer_count > 0 || base_kind == TYPE_POINTER) {
            decl->decl_type = TYPE_POINTER;
            decl->full_type = full_type;
        } else if (base_kind == TYPE_ARRAY) {
            /* Stage 28-04: variable declared with a typedef'd array type.
             * full_type is already the array Type* returned by parse_type_specifier. */
            decl->decl_type = TYPE_ARRAY;
            decl->full_type = full_type;
        } else if (base_kind == TYPE_STRUCT) {
            /* Stage 30: struct variable — carry the struct Type* for size/alignment. */
            decl->decl_type = TYPE_STRUCT;
            decl->full_type = full_type;
        } else if (base_kind == TYPE_UNION) {
            /* Stage 72: union variable. */
            decl->decl_type = TYPE_UNION;
            decl->full_type = full_type;
        } else {
            decl->decl_type = base_kind;
            /* Stage 40: mark unsigned scalar. */
            decl->is_unsigned = !base_type->is_signed;
        }
        if (parser->current.type == TOKEN_ASSIGN) {
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *init = parse_initializer(parser); /* stage 32: allows brace lists */
            if (init->type == AST_INITIALIZER_LIST &&
                decl->decl_type != TYPE_STRUCT && decl->decl_type != TYPE_UNION &&
                decl->decl_type != TYPE_ARRAY) {
                PARSER_ERROR(parser, "error: brace initializer not valid for scalar type\n");
            }
            ast_add_child(decl, init);
        }
        if (parser->current.type != TOKEN_COMMA) {
            parser_expect(parser, TOKEN_SEMICOLON);
            return decl;
        }
        /* <init_declarator_list>: one or more declarators sharing the same base type. */
        ASTNode *list = parser_node(parser, AST_DECL_LIST, NULL);
        ast_add_child(list, decl);
        while (parser->current.type == TOKEN_COMMA) {
            parser->current = lexer_next_token(parser->lexer);
            ParsedDeclarator d2 = parse_declarator(parser);
            if (d2.is_array) {
                PARSER_ERROR(parser, "error: array declarator in multi-declarator list not supported\n");
            }
            /* Stage 66: propagate const base for multi-declarator const pointer lists. */
            Type *effective_base2 = (local_is_const && d2.pointer_count > 0)
                                    ? type_const_copy(base_type) : base_type;
            Type *full_type2 = effective_base2;
            for (int i = 0; i < d2.pointer_count; i++) {
                full_type2 = type_pointer(full_type2);
            }
            ASTNode *next_decl = parser_node(parser, AST_DECLARATION, d2.name);
            next_decl->is_const = ((local_is_const && d2.pointer_count == 0) ||
                                   d2.pointer_is_const) ? 1 : 0;
            if (d2.pointer_count > 0 || base_kind == TYPE_POINTER) {
                next_decl->decl_type = TYPE_POINTER;
                next_decl->full_type = full_type2;
            } else if (base_kind == TYPE_ARRAY) {
                /* Stage 28-04: typedef'd array base — each declarator gets its
                 * own TYPE_ARRAY slot with the shared array type. */
                next_decl->decl_type = TYPE_ARRAY;
                next_decl->full_type = full_type2;
            } else if (base_kind == TYPE_STRUCT) {
                next_decl->decl_type = TYPE_STRUCT;
                next_decl->full_type = full_type2;
            } else if (base_kind == TYPE_UNION) {
                next_decl->decl_type = TYPE_UNION;
                next_decl->full_type = full_type2;
            } else {
                next_decl->decl_type = base_kind;
                next_decl->is_unsigned = !base_type->is_signed;
            }
            if (parser->current.type == TOKEN_ASSIGN) {
                parser->current = lexer_next_token(parser->lexer);
                ASTNode *init2 = parse_assignment_expression(parser);
                ast_add_child(next_decl, init2);
            }
            ast_add_child(list, next_decl);
        }
        parser_expect(parser, TOKEN_SEMICOLON);
        return list;
    }
    if (parser->current.type == TOKEN_RETURN) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *stmt = parser_node(parser, AST_RETURN_STATEMENT, NULL);
        if (parser->current.type == TOKEN_SEMICOLON) {
            /* bare return; — no expression child */
            parser->current = lexer_next_token(parser->lexer);
        } else {
            ASTNode *expr = parse_expression(parser);
            parser_expect(parser, TOKEN_SEMICOLON);
            ast_add_child(stmt, expr);
        }
        return stmt;
    }
    if (parser->current.type == TOKEN_IF) {
        return parse_if_statement(parser);
    }
    if (parser->current.type == TOKEN_WHILE) {
        return parse_while_statement(parser);
    }
    if (parser->current.type == TOKEN_DO) {
        return parse_do_while_statement(parser);
    }
    if (parser->current.type == TOKEN_FOR) {
        return parse_for_statement(parser);
    }
    if (parser->current.type == TOKEN_SWITCH) {
        return parse_switch_statement(parser);
    }
    if (parser->current.type == TOKEN_CASE) {
        if (parser->switch_depth == 0) {
            PARSER_ERROR(parser, "error: 'case' label outside of switch\n");
        }
        parser->current = lexer_next_token(parser->lexer);
        long case_val = eval_const_expr(parser, "case label expression");
        parser_expect(parser, TOKEN_COLON);
        char case_buf[32];
        snprintf(case_buf, sizeof(case_buf), "%ld", case_val);
        ASTNode *node = parser_node(parser, AST_CASE_SECTION, NULL);
        ast_add_child(node, parser_node(parser, AST_INT_LITERAL,
            lexer_store_bytes(parser->lexer, case_buf, strlen(case_buf))));
        ast_add_child(node, parse_statement(parser));
        return node;
    }
    if (parser->current.type == TOKEN_DEFAULT) {
        if (parser->switch_depth == 0) {
            PARSER_ERROR(parser, "error: 'default' label outside of switch\n");
        }
        parser->current = lexer_next_token(parser->lexer);
        parser_expect(parser, TOKEN_COLON);
        ASTNode *node = parser_node(parser, AST_DEFAULT_SECTION, NULL);
        ast_add_child(node, parse_statement(parser));
        return node;
    }
    if (parser->current.type == TOKEN_LBRACE) {
        return parse_block(parser);
    }
    if (parser->current.type == TOKEN_BREAK) {
        if (parser->loop_depth == 0 && parser->switch_depth == 0) {
            PARSER_ERROR(parser, "error: break outside of loop or switch\n");
        }
        parser->current = lexer_next_token(parser->lexer);
        parser_expect(parser, TOKEN_SEMICOLON);
        return parser_node(parser, AST_BREAK_STATEMENT, NULL);
    }
    if (parser->current.type == TOKEN_CONTINUE) {
        if (parser->loop_depth == 0) {
            PARSER_ERROR(parser, "error: continue outside of loop\n");
        }
        parser->current = lexer_next_token(parser->lexer);
        parser_expect(parser, TOKEN_SEMICOLON);
        return parser_node(parser, AST_CONTINUE_STATEMENT, NULL);
    }
    if (parser->current.type == TOKEN_GOTO) {
        parser->current = lexer_next_token(parser->lexer);
        Token name = parser_expect(parser, TOKEN_IDENTIFIER);
        parser_expect(parser, TOKEN_SEMICOLON);
        return parser_node(parser, AST_GOTO_STATEMENT, name.value);
    }
    /* expression_stmt (includes assignments, since assignment is now an expression) */
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);
    ASTNode *stmt = parser_node(parser, AST_EXPRESSION_STMT, NULL);
    ast_add_child(stmt, expr);
    return stmt;
}

/*
 * <parameter_declarator> ::= <type_specifier> [ <declarator> ]
 *
 * Stage 26: the declarator is optional — when the next token is "," or ")",
 * the parameter is unnamed and only the base type is recorded.  Unnamed
 * parameters are allowed in function prototypes and function pointer
 * signatures; function definitions require named parameters (enforced in
 * parse_external_declaration).
 *
 * Stage 25-01: function-pointer parameters are handled here via the
 * ParsedDeclarator.is_func_pointer flag set by parse_declarator.
 */
static ASTNode *parse_parameter_declaration(Parser *parser) {
    /* Stage 39: consume optional leading const qualifier on parameter types.
     * Stage 82-04: also consume optional volatile qualifier.
     * Stage 106: also consume optional restrict qualifier.
     * Stage 138: also consume optional register storage-class specifier. */
    if (parser->current.type == TOKEN_CONST     ||
        parser->current.type == TOKEN_VOLATILE   ||
        parser->current.type == TOKEN_RESTRICT   ||
        parser->current.type == TOKEN_REGISTER)
        parser->current = lexer_next_token(parser->lexer);
    TypeKind base_kind;
    Type *base_type = parse_type_specifier(parser, &base_kind);
    /* Stage 159: C99 §6.7 allows qualifiers and type-specifiers to intermix,
     * so 'void const *p' and 'int volatile x' are valid (qualifier after base).
     * Consume any trailing qualifiers silently (they are already represented
     * in the type; re-applying them here would duplicate, not add, semantics). */
    while (parser->current.type == TOKEN_CONST   ||
           parser->current.type == TOKEN_VOLATILE ||
           parser->current.type == TOKEN_RESTRICT)
        parser->current = lexer_next_token(parser->lexer);

    /* Optional declarator: absent when next token is "," or ")". */
    if (parser->current.type == TOKEN_COMMA ||
        parser->current.type == TOKEN_RPAREN) {
        ASTNode *param = parser_node(parser, AST_PARAM, "");
        if (base_kind == TYPE_ARRAY) {
            /* C99 6.7.5.3p7: unnamed array param is adjusted to pointer-to-element. */
            param->decl_type = TYPE_POINTER;
            param->full_type = type_pointer(base_type->base);
        } else {
            param->decl_type = base_kind;
        }
        return param;
    }

    /* Stage 45: C99 abstract pointer declarator in a prototype, e.g.
     * `int puts(char *)`. Pre-consume leading stars so we can detect the
     * unnamed-pointer-parameter form before delegating to parse_declarator,
     * which requires an identifier. */
    int leading_stars = 0;
    while (parser->current.type == TOKEN_STAR) {
        leading_stars++;
        parser->current = lexer_next_token(parser->lexer);
        /* Stage 106: consume optional pointer qualifiers after each star so
         * that patterns like "char * restrict s" parse correctly when the
         * leading stars are pre-consumed before parse_declarator is called. */
        while (parser->current.type == TOKEN_CONST   ||
               parser->current.type == TOKEN_VOLATILE ||
               parser->current.type == TOKEN_RESTRICT)
            parser->current = lexer_next_token(parser->lexer);
    }
    if (leading_stars > 0 &&
        (parser->current.type == TOKEN_COMMA ||
         parser->current.type == TOKEN_RPAREN)) {
        ASTNode *param = parser_node(parser, AST_PARAM, "");
        Type *full_type = base_type;
        for (int i = 0; i < leading_stars; i++)
            full_type = type_pointer(full_type);
        param->decl_type = TYPE_POINTER;
        param->full_type = full_type;
        return param;
    }

    /* C99 §6.7.5.3p7: unnamed array parameter 'T[N]' — skip the dimension(s)
     * and adjust to pointer-to-T, same as 'T *'. */
    if (leading_stars == 0 && parser->current.type == TOKEN_LBRACKET) {
        while (parser->current.type == TOKEN_LBRACKET) {
            parser->current = lexer_next_token(parser->lexer);
            int depth = 1;
            while (depth > 0 && parser->current.type != TOKEN_EOF) {
                if      (parser->current.type == TOKEN_LBRACKET) depth++;
                else if (parser->current.type == TOKEN_RBRACKET) depth--;
                parser->current = lexer_next_token(parser->lexer);
            }
        }
        ASTNode *param = parser_node(parser, AST_PARAM, "");
        param->decl_type = TYPE_POINTER;
        param->full_type = type_pointer(base_type);
        return param;
    }

    ParsedDeclarator d = parse_declarator(parser);
    /* Any stars we pre-consumed contribute to the declarator's pointer
     * count. parse_declarator saw zero leading stars for this declarator. */
    d.pointer_count += leading_stars;
    ASTNode *param = parser_node(parser, AST_PARAM, d.name);
    if (d.is_func_pointer) {
        param->decl_type = TYPE_POINTER;
        param->full_type = build_fp_type(base_type, &d);
        return param;
    }
    /* Stage 135: C99 6.7.5.3p8 — a parameter declared with function type is
     * adjusted to pointer-to-function.  The "identifier(params)" form sets
     * d.is_function; the "(" is not consumed by parse_declarator, so consume
     * the parameter list here and discard it (we only need TYPE_POINTER). */
    if (d.is_function) {
        int depth = 1;
        parser_expect(parser, TOKEN_LPAREN);
        while (depth > 0 && parser->current.type != TOKEN_EOF) {
            if (parser->current.type == TOKEN_LPAREN) depth++;
            else if (parser->current.type == TOKEN_RPAREN) depth--;
            if (depth > 0)
                parser->current = lexer_next_token(parser->lexer);
        }
        parser_expect(parser, TOKEN_RPAREN);
        param->decl_type = TYPE_POINTER;
        param->full_type = type_pointer(type_function(base_type, 0, NULL));
        return param;
    }
    /* Stage 135: C99 §6.7.5.2 — int (*row)[N] is a pointer-to-array parameter.
     * Build array type from the bound (0 for incomplete []) then wrap in pointer. */
    if (d.is_ptr_to_array) {
        Type *arr_type = type_array(base_type, d.ptr_to_array_length);
        Type *ptr_type = arr_type;
        int levels = d.pointer_count > 0 ? d.pointer_count : 1;
        int i;
        for (i = 0; i < levels; i++)
            ptr_type = type_pointer(ptr_type);
        param->decl_type = TYPE_POINTER;
        param->full_type = ptr_type;
        return param;
    }
    Type *full_type = base_type;
    for (int i = 0; i < d.pointer_count; i++) {
        full_type = type_pointer(full_type);
    }
    if (d.pointer_count > 0) {
        param->decl_type = TYPE_POINTER;
        param->full_type = full_type;
    } else if (d.is_array) {
        /* Stage 135: C99 6.7.5.3p7 — named array parameter (e.g. int a[3] or
         * int a[]) is adjusted to pointer-to-element so that int a[3], int a[],
         * and int *a all produce the same adjusted type TYPE_POINTER. */
        param->decl_type = TYPE_POINTER;
        param->full_type = type_pointer(base_type);
    } else if (base_kind == TYPE_ARRAY) {
        /* C99 6.7.5.3p7: unnamed array param is adjusted to pointer-to-element. */
        param->decl_type = TYPE_POINTER;
        param->full_type = type_pointer(base_type->base);
    } else {
        param->decl_type = base_kind;
        param->is_unsigned = !base_type->is_signed;
        /* Stage 91-01: a struct/union passed by value needs its full type
         * so codegen knows the object's size and field layout. */
        if (base_kind == TYPE_STRUCT || base_kind == TYPE_UNION)
            param->full_type = base_type;
    }
    return param;
}

/*
 * <parameter_list> ::= <parameter_declarator> { "," <parameter_declarator> }
 *
 * Parses the parameter list and appends each AST_PARAM node as a child of
 * `func`. Enforces unique parameter names within the list; unnamed
 * parameters (empty name) are exempt from the duplicate check.
 */
static void parse_parameter_list(Parser *parser, ASTNode *func) {
    ASTNode *param = parse_parameter_declaration(parser);
    ast_add_child(func, param);
    while (parser->current.type == TOKEN_COMMA) {
        parser->current = lexer_next_token(parser->lexer);
        /* Stage 57-03: `...` at end of parameter list marks the function variadic. */
        if (parser->current.type == TOKEN_ELLIPSIS) {
            parser->current = lexer_next_token(parser->lexer);
            func->is_variadic = 1;
            break;
        }
        ASTNode *next = parse_parameter_declaration(parser);
        if (next->value[0] != '\0') {
            for (int i = 0; i < func->child_count; i++) {
                if (func->children[i]->type == AST_PARAM &&
                    func->children[i]->value[0] != '\0' &&
                    strcmp(func->children[i]->value, next->value) == 0) {
                    PARSER_ERROR(parser, 
                            "error: duplicate parameter '%s' in function '%s'\n",
                            next->value, func->value);
                }
            }
        }
        ast_add_child(func, next);
    }
}

/*
 * Stage 27: collect declaration specifiers as a list, validate semantics.
 *
 * <declaration_specifiers> ::= <declaration_specifier> { <declaration_specifier> }
 * <declaration_specifier>  ::= <storage_class_specifier> | <type_specifier> | <type_qualifier>
 * <type_qualifier>         ::= "const"
 *
 * Errors: duplicate storage class, duplicate type specifier, missing type specifier.
 */
typedef struct {
    StorageClass sc;
    TypeKind base_kind;
    Type *base_type;
    int is_const;
    int is_volatile;
} DeclSpecResult;

static DeclSpecResult parse_declaration_specifiers(Parser *parser) {
    DeclSpecResult r;
    r.sc = SC_NONE;
    r.base_kind = TYPE_INT;
    r.base_type = NULL;
    r.is_const = 0;
    r.is_volatile = 0;
    int has_sc = 0;
    int has_type = 0;

    while (1) {
        if (parser->current.type == TOKEN_CONST) {
            r.is_const = 1;
            parser->current = lexer_next_token(parser->lexer);
        } else if (parser->current.type == TOKEN_VOLATILE) {
            r.is_volatile = 1;
            parser->current = lexer_next_token(parser->lexer);
        } else if (parser->current.type == TOKEN_INLINE) {
            /* C99 §6.7.4 function-specifier: parse and ignore (no codegen effect) */
            parser->current = lexer_next_token(parser->lexer);
        } else if (parser->current.type == TOKEN_AUTO) {
            PARSER_ERROR(parser,
                    "error: auto is not valid at file scope\n");
        } else if (parser->current.type == TOKEN_REGISTER) {
            PARSER_ERROR(parser,
                    "error: register is not valid at file scope\n");
        } else if (parser->current.type == TOKEN_EXTERN ||
            parser->current.type == TOKEN_STATIC ||
            parser->current.type == TOKEN_TYPEDEF) {
            if (has_sc) {
                PARSER_ERROR(parser,
                        "error: multiple storage class specifiers are not allowed\n");
            }
            has_sc = 1;
            if (parser->current.type == TOKEN_EXTERN) r.sc = SC_EXTERN;
            else if (parser->current.type == TOKEN_STATIC) r.sc = SC_STATIC;
            else r.sc = SC_TYPEDEF;
            parser->current = lexer_next_token(parser->lexer);
        } else if (parser->current.type == TOKEN_VOID ||
                   parser->current.type == TOKEN_FLOAT ||
                   parser->current.type == TOKEN_DOUBLE ||
                   parser->current.type == TOKEN_BOOL ||
                   parser->current.type == TOKEN_CHAR ||
                   parser->current.type == TOKEN_SHORT ||
                   parser->current.type == TOKEN_INT ||
                   parser->current.type == TOKEN_LONG ||
                   parser->current.type == TOKEN_SIGNED ||
                   parser->current.type == TOKEN_UNSIGNED ||
                   parser->current.type == TOKEN_ENUM ||
                   parser->current.type == TOKEN_STRUCT ||
                   parser->current.type == TOKEN_UNION ||
                   (!has_type &&
                    parser->current.type == TOKEN_IDENTIFIER &&
                    parser_find_typedef(parser, parser->current.value))) {
            if (has_type) {
                PARSER_ERROR(parser,
                        "error: multiple type specifiers are not allowed\n");
            }
            has_type = 1;
            r.base_type = parse_type_specifier(parser, &r.base_kind);
        } else {
            break;
        }
    }

    if (!has_type) {
        PARSER_ERROR(parser, "error: expected type specifier\n");
    }

    return r;
}

/*
 * <external_declaration> ::= <function_definition>
 *                           | <declaration>
 *
 * <function_definition>    ::= <declaration_specifiers> <declarator> <block_statement>
 * <declaration>            ::= <declaration_specifiers> <init_declarator_list> ";"
 * <declaration_specifiers> ::= <declaration_specifier> { <declaration_specifier> }
 * <declaration_specifier>  ::= <storage_class_specifier> | <type_specifier> | <type_qualifier>
 * <storage_class_specifier>::= "extern" | "static"
 * <type_qualifier>         ::= "const"
 *
 * After parsing the common declaration_specifiers + declarator prefix:
 *   - function declarator + "{"  → function definition
 *   - function declarator + "="  → error (no initializer on function declaration)
 *   - function declarator + ";"  → function declaration (prototype)
 *   - non-function declarator + "{" → error (must be function declarator)
 *   - non-function declarator       → file-scope object declaration
 *
 * AST layout for a definition: zero or more AST_PARAM children followed by
 * the AST_BLOCK body. A pure declaration has only the AST_PARAM children
 * (no AST_BLOCK).
 */
static ASTNode *parse_external_declaration(Parser *parser) {
    DeclSpecResult ds = parse_declaration_specifiers(parser);
    StorageClass sc = ds.sc;
    TypeKind base_kind = ds.base_kind;
    Type *base_type = ds.base_type;

    /* Stage 29: standalone type declaration with no declarator (e.g. "enum E{};"). */
    if (parser->current.type == TOKEN_SEMICOLON) {
        parser_expect(parser, TOKEN_SEMICOLON);
        return parser_node(parser, AST_TYPEDEF_DECL, "");
    }

    ParsedDeclarator d = parse_declarator(parser);

    /* Stage 137: function returning pointer-to-function — int (*name())(params).
     * Both the own param list and the fp param list were consumed inline by
     * parse_declarator; build the return type and function node here. */
    if (d.is_func_returning_fp) {
        /* Build return type: fp_outer_stars + base → function(fp_params) →
         * wrap fp_inner_stars times in type_pointer. */
        Type *fp_ret = base_type;
        int i;
        for (i = 0; i < d.fp_outer_stars; i++)
            fp_ret = type_pointer(fp_ret);
        Type *fp_func_type = type_function(fp_ret, d.fp_param_count,
                                           (Type **)d.fp_param_types);
        Type *fp_full_type = fp_func_type;
        for (i = 0; i < d.fp_inner_stars; i++)
            fp_full_type = type_pointer(fp_full_type);

        ASTNode *func = parser_node(parser, AST_FUNCTION_DECL, d.name);
        func->decl_type  = TYPE_POINTER;
        func->full_type  = fp_full_type;
        func->storage_class = sc;
        func->is_no_prototype = d.own_is_no_prototype;

        /* Create unnamed AST_PARAM children from own parameter types.
         * Definitions with non-empty own params require named params — reject
         * if any are anonymous (the spec's tests use empty own params). */
        for (i = 0; i < d.own_param_count; i++) {
            ASTNode *p = parser_node(parser, AST_PARAM, "");
            p->decl_type = d.own_param_types[i]->kind;
            if (d.own_param_types[i]->kind == TYPE_POINTER)
                p->full_type = d.own_param_types[i];
            ast_add_child(func, p);
        }

        int param_count = d.own_param_count;
        TypeKind own_ptypes[FUNC_MAX_PARAMS];
        for (i = 0; i < param_count; i++)
            own_ptypes[i] = d.own_param_types[i]->kind;
        int is_definition = (parser->current.type == TOKEN_LBRACE);

        if (is_definition && param_count > 0) {
            for (i = 0; i < param_count; i++) {
                if (func->children[i]->value[0] == '\0') {
                    PARSER_ERROR(parser,
                        "error: unnamed parameter in definition of '%s'\n",
                        d.name);
                }
            }
        }
        parser_register_function(parser, d.name, param_count, is_definition,
                                 TYPE_POINTER, own_ptypes, sc, 0,
                                 d.own_is_no_prototype);
        if (is_definition) {
            int sv = parser->current_func_is_variadic;
            const char *sfn = parser->current_func_name;
            parser->current_func_is_variadic = 0;
            parser->current_func_name = d.name;
            ast_add_child(func, parse_block(parser));
            parser->current_func_is_variadic = sv;
            parser->current_func_name = sfn;
        } else {
            parser_expect(parser, TOKEN_SEMICOLON);
        }
        return func;
    }

    /* Stage 28-01/28-02/28-03/28-04: typedef declaration at file scope. */
    if (sc == SC_TYPEDEF) {
        if (d.is_function) {
            /* Function-type typedef: 'typedef ret name(params);'
             * Skip the parameter list and register name as a long-sized type so
             * 'name *fp' is accepted as a function-pointer-sized field in structs. */
            {
                parser_expect(parser, TOKEN_LPAREN);
                int depth = 1;
                while (depth > 0 && parser->current.type != TOKEN_EOF) {
                    if      (parser->current.type == TOKEN_LPAREN) depth++;
                    else if (parser->current.type == TOKEN_RPAREN) depth--;
                    parser->current = lexer_next_token(parser->lexer);
                }
            }
            parser_expect(parser, TOKEN_SEMICOLON);
            parser_register_typedef(parser, d.name, TYPE_LONG, NULL);
            return parser_node(parser, AST_TYPEDEF_DECL, d.name);
        }
        /* Stage 28-04: array typedef — register with the full array Type*.
         * Stage 86: multi-dimensional array typedefs. */
        if (d.is_array) {
            if (!d.has_size) {
                PARSER_ERROR(parser, "error: array typedef requires explicit size\n");
            }
            parser_expect(parser, TOKEN_SEMICOLON);
            Type *array_type = build_array_type_from_dims(base_type,
                                                          d.array_dims, d.array_dim_count);
            parser_register_typedef(parser, d.name, TYPE_ARRAY, array_type);
            return parser_node(parser, AST_TYPEDEF_DECL, d.name);
        }
        if (parser->current.type == TOKEN_ASSIGN) {
            PARSER_ERROR(parser,
                    "error: typedef declaration cannot have an initializer\n");
        }
        parser_expect(parser, TOKEN_SEMICOLON);
        if (d.is_func_pointer) {
            Type *fp_type = build_fp_type(base_type, &d);
            parser_register_typedef(parser, d.name, TYPE_POINTER, fp_type);
            return parser_node(parser, AST_TYPEDEF_DECL, d.name);
        }
        Type *full_type = base_type;
        for (int i = 0; i < d.pointer_count; i++)
            full_type = type_pointer(full_type);
        TypeKind typedef_kind = (d.pointer_count > 0 || base_kind == TYPE_POINTER)
                                ? TYPE_POINTER : base_kind;
        /* Stage 40: for unsigned scalar typedefs, store the base Type* so
         * signedness is preserved when the typedef name is later resolved. */
        Type *reg_full_type = (typedef_kind == TYPE_POINTER ||
                               typedef_kind == TYPE_STRUCT  ||
                               typedef_kind == TYPE_UNION)
                              ? full_type
                              : (!base_type->is_signed ? base_type : NULL);
        parser_register_typedef(parser, d.name, typedef_kind, reg_full_type);
        return parser_node(parser, AST_TYPEDEF_DECL, d.name);
    }

    /* Stage 25-01/25-02: function-pointer file-scope declaration. */
    if (d.is_func_pointer) {
        Type *fp_type = build_fp_type(base_type, &d);
        if (sc == SC_EXTERN && parser->current.type == TOKEN_ASSIGN) {
            PARSER_ERROR(parser, 
                    "error: extern declaration of '%s' cannot have an initializer\n",
                    d.name);
        }
        if (parser_find_function(parser, d.name)) {
            PARSER_ERROR(parser, 
                    "error: '%s' redeclared as a different kind of symbol\n", d.name);
        }
        parser_register_global(parser, d.name, TYPE_POINTER, sc, fp_type,
                               parser->current.type == TOKEN_ASSIGN);
        ASTNode *decl = parser_node(parser, AST_DECLARATION, d.name);
        decl->storage_class = sc;
        decl->decl_type = TYPE_POINTER;
        decl->full_type = fp_type;
        /* Stage 25-02: optional function-designator initializer. */
        if (parser->current.type == TOKEN_ASSIGN) {
            parser->current = lexer_next_token(parser->lexer);
            if (parser->current.type != TOKEN_IDENTIFIER) {
                PARSER_ERROR(parser, 
                        "error: function pointer initializer must be a function name\n");
            }
            Token init_tok = parser->current;
            parser->current = lexer_next_token(parser->lexer);
            FuncSig *sig = parser_find_function(parser, init_tok.value);
            if (!sig) {
                PARSER_ERROR(parser, 
                        "error: '%s' is not a declared function\n", init_tok.value);
            }
            /* Verify compatibility against the declared function pointer type. */
            Type *fp_fn = fp_type->base; /* TYPE_FUNCTION node */
            if (fp_fn->base->kind != sig->return_type ||
                fp_fn->param_count != sig->param_count) {
                PARSER_ERROR(parser, 
                        "error: incompatible function pointer type in initializer of '%s'\n",
                        d.name);
            }
            for (int i = 0; i < sig->param_count; i++) {
                if (!fp_fn->params[i] ||
                    fp_fn->params[i]->kind != sig->param_types[i]) {
                    PARSER_ERROR(parser, 
                            "error: incompatible function pointer type in initializer of '%s'\n",
                            d.name);
                }
            }
            ASTNode *init_node = parser_node(parser, AST_VAR_REF, init_tok.value);
            ast_add_child(decl, init_node);
        }
        parser_expect(parser, TOKEN_SEMICOLON);
        return decl;
    }

    if (!d.is_function) {
        if (parser->current.type == TOKEN_LBRACE) {
            PARSER_ERROR(parser, "error: '%s' is not a function declarator\n", d.name);
        }
        /* Reject `void g;` — void cannot be an object type. */
        if (base_kind == TYPE_VOID && d.pointer_count == 0 &&
            !d.is_func_pointer && !d.is_array) {
            PARSER_ERROR(parser, 
                    "error: cannot declare variable '%s' of type void\n", d.name);
        }
        /* Stage 66: propagate const base for file-scope const pointer declarations. */
        Type *effective_base_fs = base_type;
        if (ds.is_const && d.pointer_count > 0)
            effective_base_fs = type_const_copy(base_type);
        else if (ds.is_volatile && d.pointer_count > 0)
            effective_base_fs = type_volatile_copy(base_type);
        Type *full_type = effective_base_fs;
        for (int i = 0; i < d.pointer_count; i++)
            full_type = type_pointer(full_type);

        /* Stage 23: extern with initializer is invalid. */
        if (sc == SC_EXTERN && parser->current.type == TOKEN_ASSIGN) {
            PARSER_ERROR(parser, 
                    "error: extern declaration of '%s' cannot have an initializer\n",
                    d.name);
        }

        /* Stage 22-02/23: detect function/object name conflicts and register. */
        if (parser_find_function(parser, d.name)) {
            PARSER_ERROR(parser, 
                    "error: '%s' redeclared as a different kind of symbol\n", d.name);
        }
        TypeKind obj_kind = d.is_array ? TYPE_ARRAY :
                            (d.pointer_count > 0 || base_kind == TYPE_POINTER) ? TYPE_POINTER : base_kind;
        Type *reg_full_type = (obj_kind == TYPE_POINTER ||
                               obj_kind == TYPE_STRUCT  ||
                               obj_kind == TYPE_UNION)  ? full_type : NULL;
        parser_register_global(parser, d.name, obj_kind, sc, reg_full_type,
                               parser->current.type == TOKEN_ASSIGN);

        ASTNode *decl = parser_node(parser, AST_DECLARATION, d.name);
        decl->storage_class = sc;
        /* Stage 39/66: const applies to the scalar variable when no pointer depth,
         * or to the pointer variable itself when T * const p form is used. */
        decl->is_const = ((ds.is_const && d.pointer_count == 0 && !d.is_array) ||
                          d.pointer_is_const) ? 1 : 0;
        if (d.is_array) {
            /* Stage 43: file-scope array with optional initializer.
             * Size may be inferred from a string literal or brace list. */
            int has_size = d.has_size;
            int length = d.array_length;
            ASTNode *init_node = NULL;

            if (parser->current.type == TOKEN_ASSIGN) {
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_LBRACE) {
                    init_node = parse_initializer(parser);
                    if (!has_size) {
                        length = init_node->child_count;
                        has_size = 1;
                    } else if (init_node->child_count > length) {
                        /* Stage 44: too many initializers for explicitly-sized array. */
                        PARSER_ERROR(parser, 
                                "error: too many initializers for array '%s'\n", d.name);
                    }
                } else if (parser->current.type == TOKEN_STRING_LITERAL) {
                    if (full_type->kind != TYPE_CHAR) {
                        PARSER_ERROR(parser, 
                                "error: string initializer only supported for char arrays\n");
                    }
                    Token str_tok = parser->current;
                    parser->current = lexer_next_token(parser->lexer);
                    ASTNode *str_init = parser_node(parser, AST_STRING_LITERAL, NULL);
                    str_init->value = str_tok.value;
                    str_init->byte_length = (int)str_tok.value_len;
                    int needed = str_init->byte_length + 1;
                    if (has_size) {
                        if (length < needed) {
                            PARSER_ERROR(parser,
                                    "error: array too small for string literal initializer\n");
                        }
                    } else {
                        length = needed;
                    }
                    init_node = str_init;
                } else {
                    PARSER_ERROR(parser, 
                            "error: array initializer must be a brace-enclosed list or string literal\n");
                }
            } else if (!has_size) {
                if (sc != SC_EXTERN) {
                    PARSER_ERROR(parser,
                            "error: array size required for file-scope declaration '%s'\n",
                            d.name);
                }
                /* Stage 129: extern incomplete array — treat as zero-length for codegen.
                 * Emitted as 'extern name' in the assembly output. */
                length = 0;
                has_size = 1;
            }

            /* Stage 86: build multi-dim type; update array_dims[0] with inferred length. */
            d.array_dims[0] = length;
            if (d.array_dim_count == 0) d.array_dim_count = 1;
            decl->decl_type = TYPE_ARRAY;
            decl->full_type = build_array_type_from_dims(full_type,
                                                         d.array_dims, d.array_dim_count);
            if (init_node)
                ast_add_child(decl, init_node);
            parser_expect(parser, TOKEN_SEMICOLON);
            return decl;
        }
        if (d.pointer_count > 0 || base_kind == TYPE_POINTER) {
            decl->decl_type = TYPE_POINTER;
            decl->full_type = full_type;
        } else if (base_kind == TYPE_ARRAY) {
            /* Stage 28-04: file-scope variable declared with a typedef'd array
             * type (e.g. `typedef long jmp_buf[32]; jmp_buf g;`). The declarator
             * carries no `[]`, so d.is_array is false and the array-ness comes
             * from the typedef; full_type is already the array Type* returned by
             * parse_type_specifier. Mirror the block-scope path so codegen finds
             * the element type instead of dereferencing a NULL full_type. */
            decl->decl_type = TYPE_ARRAY;
            decl->full_type = full_type;
        } else if (base_kind == TYPE_STRUCT) {
            decl->decl_type = TYPE_STRUCT;
            decl->full_type = full_type;
        } else if (base_kind == TYPE_UNION) {
            /* Stage 72: union global variable. */
            decl->decl_type = TYPE_UNION;
            decl->full_type = full_type;
        } else {
            decl->decl_type = base_kind;
            /* Stage 40: mark unsigned scalar. */
            decl->is_unsigned = !base_type->is_signed;
        }
        /* Stage 22-02: optional constant initializer.
         * Stage 43: also accept string literals for pointer globals.
         * Stage 44: accept brace-list initializers for struct globals.
         * Stage 72: accept brace-list initializers for union globals (first member). */
        if (parser->current.type == TOKEN_ASSIGN) {
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *init;
            if (parser->current.type == TOKEN_LBRACE) {
                /* Stage 44/72: struct or union global initialized with brace list. */
                if (decl->decl_type != TYPE_STRUCT && decl->decl_type != TYPE_UNION) {
                    PARSER_ERROR(parser,
                            "error: brace initializer not valid for non-struct global '%s'\n",
                            d.name);
                }
                init = parse_initializer(parser);
            } else if (decl->decl_type == TYPE_FLOAT ||
                       decl->decl_type == TYPE_DOUBLE) {
                /* Stage 128: evaluate as a compile-time FP constant expression. */
                double fv = eval_fp_const_expr(parser, "float/double global initializer");
                char fp_buf[64];
                if (decl->decl_type == TYPE_FLOAT) {
                    float f32 = (float)fv;
                    snprintf(fp_buf, sizeof(fp_buf), "%.9g", (double)f32);
                } else {
                    snprintf(fp_buf, sizeof(fp_buf), "%.17g", fv);
                }
                if (!strchr(fp_buf, '.') && !strchr(fp_buf, 'e') && !strchr(fp_buf, 'E'))
                    strncat(fp_buf, ".0", sizeof(fp_buf) - strlen(fp_buf) - 1);
                init = parser_node(parser, AST_FLOAT_LITERAL,
                                   lexer_store_bytes(parser->lexer, fp_buf, strlen(fp_buf)));
            } else if (decl->decl_type != TYPE_POINTER &&
                       decl->decl_type != TYPE_STRUCT &&
                       decl->decl_type != TYPE_UNION) {
                /* Stage 100: integer-typed global — evaluate as compile-time constant. */
                long val = eval_const_expr(parser, "file-scope initializer");
                char init_buf[32];
                snprintf(init_buf, sizeof(init_buf), "%ld", val);
                init = parser_node(parser, AST_INT_LITERAL,
                           lexer_store_bytes(parser->lexer, init_buf, strlen(init_buf)));
            } else {
                /* Pointer or aggregate global — keep literal/expression path. */
                init = parse_assignment_expression(parser);
                /* Stage 124: compound literals at file scope are only supported
                 * as pointer initializers (array decay or address-of). */
                if (init->type == AST_COMPOUND_LITERAL &&
                    decl->decl_type != TYPE_POINTER) {
                    PARSER_ERROR(parser,
                            "error: compound literals at file scope are not yet supported\n");
                }
                /* Accept integer/char/string literals, function designators (VAR_REF),
                 * address constants (&global, &global[N]), compound literals
                 * for pointer globals (Stage 124), and null pointer constant casts
                 * like (void *)0 from NULL when defined as ((void *)0) (Stage 163). */
                {
                    int is_null_cast = (init->type == AST_CAST &&
                                        init->decl_type == TYPE_POINTER &&
                                        init->child_count > 0 &&
                                        init->children[0]->type == AST_INT_LITERAL &&
                                        strcmp(init->children[0]->value, "0") == 0);
                    if (init->type != AST_INT_LITERAL && init->type != AST_CHAR_LITERAL &&
                        init->type != AST_STRING_LITERAL && init->type != AST_VAR_REF &&
                        init->type != AST_ADDR_OF &&
                        init->type != AST_COMPOUND_LITERAL &&
                        !is_null_cast) {
                        PARSER_ERROR(parser,
                                "error: non-constant initializer for global '%s'\n", d.name);
                    }
                }
            }
            ast_add_child(decl, init);
        }
        if (parser->current.type != TOKEN_COMMA) {
            parser_expect(parser, TOKEN_SEMICOLON);
            return decl;
        }
        /* Stage 22-02: comma-separated declarator list at file scope. */
        ASTNode *list = parser_node(parser, AST_DECL_LIST, NULL);
        ast_add_child(list, decl);
        while (parser->current.type == TOKEN_COMMA) {
            parser->current = lexer_next_token(parser->lexer);
            ParsedDeclarator d2 = parse_declarator(parser);
            if (d2.is_array || d2.is_function) {
                PARSER_ERROR(parser, 
                        "error: invalid declarator in file-scope list\n");
            }
            if (parser_find_function(parser, d2.name)) {
                PARSER_ERROR(parser, 
                        "error: '%s' redeclared as a different kind of symbol\n", d2.name);
            }
            TypeKind k2 = (d2.pointer_count > 0 || base_kind == TYPE_POINTER)
                          ? TYPE_POINTER : base_kind;
            /* Stage 66: propagate const base for file-scope multi-declarator lists.
             * Stage 82-04: same for volatile. */
            Type *eff_base2 = base_type;
            if (ds.is_const && d2.pointer_count > 0)
                eff_base2 = type_const_copy(base_type);
            else if (ds.is_volatile && d2.pointer_count > 0)
                eff_base2 = type_volatile_copy(base_type);
            Type *ft2 = eff_base2;
            for (int i = 0; i < d2.pointer_count; i++)
                ft2 = type_pointer(ft2);
            Type *reg_ft2 = (k2 == TYPE_POINTER) ? ft2 : NULL;
            parser_register_global(parser, d2.name, k2, sc, reg_ft2,
                                   parser->current.type == TOKEN_ASSIGN);
            ASTNode *next_decl = parser_node(parser, AST_DECLARATION, d2.name);
            next_decl->storage_class = sc;
            next_decl->is_const = ((ds.is_const && d2.pointer_count == 0) ||
                                   d2.pointer_is_const) ? 1 : 0;
            if (d2.pointer_count > 0 || base_kind == TYPE_POINTER) {
                next_decl->decl_type = TYPE_POINTER;
                next_decl->full_type = ft2;
            } else {
                next_decl->decl_type = base_kind;
                next_decl->is_unsigned = !base_type->is_signed;
            }
            if (parser->current.type == TOKEN_ASSIGN) {
                parser->current = lexer_next_token(parser->lexer);
                ASTNode *init2;
                if (k2 != TYPE_POINTER) {
                    /* Stage 100: integer-typed — evaluate as compile-time constant. */
                    long val2 = eval_const_expr(parser, "file-scope initializer");
                    char init2_buf[32];
                    snprintf(init2_buf, sizeof(init2_buf), "%ld", val2);
                    init2 = parser_node(parser, AST_INT_LITERAL,
                                lexer_store_bytes(parser->lexer, init2_buf, strlen(init2_buf)));
                } else {
                    init2 = parse_primary(parser);
                    if (init2->type != AST_INT_LITERAL && init2->type != AST_CHAR_LITERAL) {
                        PARSER_ERROR(parser,
                                "error: non-constant initializer for global '%s'\n", d2.name);
                    }
                }
                ast_add_child(next_decl, init2);
            }
            ast_add_child(list, next_decl);
        }
        parser_expect(parser, TOKEN_SEMICOLON);
        return list;
    }

    Type *full_type = base_type;
    for (int i = 0; i < d.pointer_count; i++)
        full_type = type_pointer(full_type);
    TypeKind return_kind = (d.pointer_count > 0) ? TYPE_POINTER : base_kind;
    ASTNode *func = parser_node(parser, AST_FUNCTION_DECL, d.name);
    func->decl_type = return_kind;
    func->storage_class = sc;
    /* Stage 137: use return_kind instead of d.pointer_count so that typedef'd
     * pointer return types (base_kind == TYPE_POINTER, d.pointer_count == 0)
     * also set func->full_type. */
    if (return_kind == TYPE_POINTER)
        func->full_type = full_type;
    /* Stage 91-01: a struct/union returned by value needs its full type so
     * codegen knows the object's size (and whether it is register- or
     * memory-class for the SysV AMD64 return convention). */
    else if (return_kind == TYPE_STRUCT || return_kind == TYPE_UNION)
        func->full_type = base_type;

    /* Stage 133: track whether the parameter list is empty `()` (no prototype)
     * as opposed to `(void)` (zero-parameter prototype) or a real list. */
    int is_no_prototype = 0;
    parser_expect(parser, TOKEN_LPAREN);
    /* `f(void)` — the sole `void` keyword means zero parameters. */
    if (parser->current.type == TOKEN_VOID) {
        int saved_pos = parser->lexer->pos;
        Token saved = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type != TOKEN_RPAREN) {
            /* `void` followed by `*` or identifier is a normal parameter;
             * restore and parse the full parameter list. */
            parser->lexer->pos = saved_pos;
            parser->current = saved;
            parse_parameter_list(parser, func);
        }
        /* else: true `(void)` — zero parameters; leave func with no children */
    } else if (parser->current.type != TOKEN_RPAREN) {
        parse_parameter_list(parser, func);
    } else {
        /* Empty `()` — no prototype information per C99 §6.7.5.3p14. */
        is_no_prototype = 1;
    }
    parser_expect(parser, TOKEN_RPAREN);
    func->is_no_prototype = is_no_prototype;

    if (parser->current.type == TOKEN_ASSIGN) {
        PARSER_ERROR(parser,
                "error: function declaration cannot have an initializer\n");
    }

    int param_count = func->child_count;
    int is_definition = (parser->current.type == TOKEN_LBRACE);

    /* Stage 26: function definitions require named parameters.
     * Stage 75-01: variadic definitions may have unnamed fixed parameters
     * (e.g. int f(const char *, ...)) since the callee can ignore them. */
    if (is_definition && !func->is_variadic) {
        for (int i = 0; i < param_count; i++) {
            if (func->children[i]->type == AST_PARAM &&
                func->children[i]->value[0] == '\0') {
                PARSER_ERROR(parser,
                        "error: unnamed parameter in definition of '%s'\n",
                        d.name);
            }
        }
    }

    /* Collect parameter types for registration. */
    TypeKind param_types[FUNC_MAX_PARAMS];
    if (param_count > FUNC_MAX_PARAMS) {
        PARSER_ERROR(parser, 
                "error: function '%s' has %d parameters; max supported is %d\n",
                d.name, param_count, FUNC_MAX_PARAMS);
    }
    for (int i = 0; i < param_count; i++)
        param_types[i] = func->children[i]->decl_type;

    /* Register before parsing the body so self-calls resolve. */
    parser_register_function(parser, d.name, param_count, is_definition,
                             return_kind, param_types, sc, func->is_variadic,
                             is_no_prototype);

    if (is_definition) {
        /* Stage 75-03: expose the variadic flag to nested expression parsers
         * so __builtin_va_start can enforce its "must be inside variadic
         * function" rule. */
        int         saved_variadic  = parser->current_func_is_variadic;
        const char *saved_func_name = parser->current_func_name;
        parser->current_func_is_variadic = func->is_variadic;
        parser->current_func_name        = d.name;
        ast_add_child(func, parse_block(parser));
        parser->current_func_is_variadic = saved_variadic;
        parser->current_func_name        = saved_func_name;
    } else {
        parser_expect(parser, TOKEN_SEMICOLON);
    }
    return func;
}

/*
 * <translation_unit> ::= <external_declaration> { <external_declaration> }
 *
 * Duplicate-definition and signature-consistency rules are enforced in
 * parser_register_function; multiple declarations of the same function
 * are permitted.
 *
 * When max-errors > 1 or unlimited (0), PARSER_ERROR(parser, ) longjmps back
 * here rather than exiting; parser_sync() advances to the next declaration
 * boundary so parsing can continue.
 */

/* Advance the token stream to the next top-level declaration boundary —
 * past the next ';' at depth 0, or past the next top-level '}' block.
 * Used for error recovery after a failed external declaration. */
static void parser_sync(Parser *parser) {
    int depth = 0;
    while (parser->current.type != TOKEN_EOF) {
        if (parser->current.type == TOKEN_LBRACE) {
            depth++;
        } else if (parser->current.type == TOKEN_RBRACE) {
            if (depth > 0) depth--;
            if (depth == 0) {
                parser->current = lexer_next_token(parser->lexer);
                return;
            }
        } else if (parser->current.type == TOKEN_SEMICOLON && depth == 0) {
            parser->current = lexer_next_token(parser->lexer);
            return;
        }
        parser->current = lexer_next_token(parser->lexer);
    }
}

ASTNode *parse_translation_unit(Parser *parser) {
    ASTNode *unit = parser_node(parser, AST_TRANSLATION_UNIT, NULL);
    g_error_jmp_valid = 1;
    while (parser->current.type != TOKEN_EOF) {
        if (setjmp(g_error_jmp)) {
            /* Returned via longjmp from PARSER_ERROR(parser, ).
             * Reset scope and advance past the failed declaration. */
            parser->scope_depth = 0;
            parser_sync(parser);
            continue;
        }
        ASTNode *ext_decl = parse_external_declaration(parser);
        ast_add_child(unit, ext_decl);
    }
    g_error_jmp_valid = 0;
    return unit;
}
