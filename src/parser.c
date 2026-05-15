#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

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
typedef struct {
    char name[256];
    int  pointer_count;
    int  is_array;
    int  has_size;
    int  array_length;
    int  is_function;
    int  is_func_pointer;
    int  fp_outer_stars;
    int  fp_inner_stars;
    /* Stage 26: param types consumed inline when is_func_pointer is set. */
    Type *fp_param_types[FUNC_TYPE_MAX_PARAMS];
    int   fp_param_count;
} ParsedDeclarator;

void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->current = lexer_next_token(lexer);
    parser->func_count = 0;
    parser->global_count = 0;
    parser->loop_depth = 0;
    parser->switch_depth = 0;
    parser->typedef_count = 0;
    parser->scope_depth = 0;
    parser->enum_const_count = 0;
    parser->enum_tag_count = 0;
    parser->struct_tag_count = 0;
}

/* Stage 28-01: typedef name table helpers. */
static TypedefEntry *parser_find_typedef(Parser *parser, const char *name) {
    for (int i = parser->typedef_count - 1; i >= 0; i--) {
        if (strcmp(parser->typedefs[i].name, name) == 0)
            return &parser->typedefs[i];
    }
    return NULL;
}

static void parser_register_typedef(Parser *parser, const char *name,
                                    TypeKind kind, Type *full_type) {
    for (int i = 0; i < parser->typedef_count; i++) {
        if (parser->typedefs[i].scope_depth == parser->scope_depth &&
            strcmp(parser->typedefs[i].name, name) == 0) {
            fprintf(stderr, "error: duplicate typedef '%s' in this scope\n",
                    name);
            exit(1);
        }
    }
    if (parser->typedef_count >= PARSER_MAX_TYPEDEFS) {
        fprintf(stderr, "error: too many typedefs (max %d)\n",
                PARSER_MAX_TYPEDEFS);
        exit(1);
    }
    TypedefEntry *e = &parser->typedefs[parser->typedef_count++];
    strncpy(e->name, name, sizeof(e->name) - 1);
    e->name[sizeof(e->name) - 1] = '\0';
    e->kind = kind;
    e->full_type = full_type;
    e->scope_depth = parser->scope_depth;
}

static void parser_leave_scope(Parser *parser) {
    int new_count = 0;
    for (int i = 0; i < parser->typedef_count; i++) {
        if (parser->typedefs[i].scope_depth < parser->scope_depth)
            parser->typedefs[new_count++] = parser->typedefs[i];
    }
    parser->typedef_count = new_count;
    parser->scope_depth--;
}

static FuncSig *parser_find_function(Parser *parser, const char *name) {
    for (int i = 0; i < parser->func_count; i++) {
        if (strcmp(parser->funcs[i].name, name) == 0) {
            return &parser->funcs[i];
        }
    }
    return NULL;
}

/* Stage 22-02: file-scope object tracking. */
static GlobalObjSig *parser_find_global(Parser *parser, const char *name) {
    for (int i = 0; i < parser->global_count; i++) {
        if (strcmp(parser->globals[i].name, name) == 0)
            return &parser->globals[i];
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
static void parser_register_global(Parser *parser, const char *name,
                                   TypeKind kind, StorageClass sc,
                                   Type *full_type) {
    GlobalObjSig *existing = parser_find_global(parser, name);
    if (existing) {
        if (existing->kind != kind) {
            fprintf(stderr, "error: conflicting type for global '%s'\n", name);
            exit(1);
        }
        /* Stage 25-01: for function-pointer globals, verify the full type. */
        if (kind == TYPE_POINTER && existing->full_type && full_type) {
            Type *ef = existing->full_type, *nf = full_type;
            if (ef->base && nf->base &&
                ef->base->kind == TYPE_FUNCTION &&
                nf->base->kind == TYPE_FUNCTION) {
                if (!func_ptr_types_equal(ef, nf)) {
                    fprintf(stderr,
                            "error: conflicting type for global '%s'\n", name);
                    exit(1);
                }
            }
        }
        int ex_is_static = (existing->storage_class == SC_STATIC);
        int new_is_static = (sc == SC_STATIC);
        if (ex_is_static != new_is_static) {
            fprintf(stderr, "error: conflicting linkage for '%s'\n", name);
            exit(1);
        }
        if (ex_is_static) {
            /* static + static: duplicate definition */
            fprintf(stderr, "error: duplicate global declaration '%s'\n", name);
            exit(1);
        }
        /* Both non-static: extern+extern and extern+none are OK;
         * none+none is a duplicate definition. */
        if (existing->storage_class != SC_EXTERN && sc != SC_EXTERN) {
            fprintf(stderr, "error: duplicate global declaration '%s'\n", name);
            exit(1);
        }
        /* If new declaration is a definition (SC_NONE), upgrade the entry. */
        if (sc == SC_NONE) {
            existing->storage_class = SC_NONE;
        }
        return;
    }
    if (parser->global_count >= PARSER_MAX_GLOBALS) {
        fprintf(stderr, "error: too many global objects (max %d)\n", PARSER_MAX_GLOBALS);
        exit(1);
    }
    GlobalObjSig *g = &parser->globals[parser->global_count++];
    strncpy(g->name, name, 255);
    g->name[255] = '\0';
    g->kind = kind;
    g->storage_class = sc;
    g->full_type = full_type;
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
                                     StorageClass sc) {
    /* Stage 22-02: reject function if a global object with the same name exists. */
    if (parser_find_global(parser, name)) {
        fprintf(stderr,
                "error: '%s' redeclared as a different kind of symbol\n", name);
        exit(1);
    }
    FuncSig *existing = parser_find_function(parser, name);
    if (existing) {
        /* Stage 23: check for static vs non-static linkage conflict. */
        int ex_is_static = (existing->storage_class == SC_STATIC);
        int new_is_static = (sc == SC_STATIC);
        if (ex_is_static != new_is_static) {
            fprintf(stderr,
                    "error: conflicting linkage for '%s'\n", name);
            exit(1);
        }
        if (existing->param_count != param_count) {
            fprintf(stderr,
                    "error: function '%s' parameter count mismatch (%d vs %d)\n",
                    name, existing->param_count, param_count);
            exit(1);
        }
        if (existing->return_type != return_type) {
            fprintf(stderr,
                    "error: function '%s' return type mismatch\n", name);
            exit(1);
        }
        for (int i = 0; i < param_count; i++) {
            if (existing->param_types[i] != param_types[i]) {
                fprintf(stderr,
                        "error: function '%s' parameter type mismatch at position %d\n",
                        name, i + 1);
                exit(1);
            }
        }
        if (is_definition) {
            if (existing->has_definition) {
                fprintf(stderr,
                        "error: duplicate function definition '%s'\n",
                        name);
                exit(1);
            }
            existing->has_definition = 1;
        }
        return;
    }
    if (parser->func_count >= PARSER_MAX_FUNCTIONS) {
        fprintf(stderr, "error: too many functions (max %d)\n", PARSER_MAX_FUNCTIONS);
        exit(1);
    }
    if (param_count > FUNC_MAX_PARAMS) {
        fprintf(stderr,
                "error: function '%s' has %d parameters; max supported is %d\n",
                name, param_count, FUNC_MAX_PARAMS);
        exit(1);
    }
    FuncSig *sig = &parser->funcs[parser->func_count];
    strncpy(sig->name, name, 255);
    sig->name[255] = '\0';
    sig->param_count = param_count;
    sig->has_definition = is_definition;
    sig->return_type = return_type;
    sig->storage_class = sc;
    for (int i = 0; i < param_count; i++) {
        sig->param_types[i] = param_types[i];
    }
    parser->func_count++;
}

static Token parser_expect(Parser *parser, TokenType type) {
    if (parser->current.type != type) {
        fprintf(stderr, "error: expected %s, got %s ('%s')\n",
                token_display_name(type),
                token_display_name(parser->current.type),
                parser->current.value);
        exit(1);
    }
    Token token = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    return token;
}

/* Forward declarations needed by parse_struct_specifier. */
static Type *parse_type_specifier(Parser *parser, TypeKind *out_kind);
static ParsedDeclarator parse_declarator(Parser *parser);

/* Stage 30: struct tag table helpers. */
static StructTag *parser_find_struct_tag(Parser *parser, const char *tag) {
    for (int i = 0; i < parser->struct_tag_count; i++) {
        if (strcmp(parser->struct_tags[i].tag, tag) == 0)
            return &parser->struct_tags[i];
    }
    return NULL;
}

static StructTag *parser_register_struct_tag(Parser *parser, const char *tag) {
    StructTag *st = parser_find_struct_tag(parser, tag);
    if (st) return st;
    if (parser->struct_tag_count >= PARSER_MAX_STRUCT_TAGS) {
        fprintf(stderr, "error: too many struct tags (max %d)\n",
                PARSER_MAX_STRUCT_TAGS);
        exit(1);
    }
    st = &parser->struct_tags[parser->struct_tag_count++];
    strncpy(st->tag, tag, sizeof(st->tag) - 1);
    st->tag[sizeof(st->tag) - 1] = '\0';
    st->type = NULL;
    return st;
}

/*
 * <struct_specifier> ::= "struct" <identifier> "{" <struct_declaration_list> "}"
 *                      | "struct" <identifier>
 *
 * <struct_declaration_list> ::= <struct_declaration> { <struct_declaration> }
 * <struct_declaration>      ::= <type_specifier> <struct_declarator_list> ";"
 * <struct_declarator_list>  ::= <declarator> { "," <declarator> }
 *
 * Layout uses natural alignment: each field is aligned to its type's natural
 * alignment; the struct's total size is rounded up to the struct's alignment
 * (the maximum field alignment). Returns the TYPE_STRUCT Type*.
 */
static Type *parse_struct_specifier(Parser *parser) {
    /* consume 'struct' */
    parser->current = lexer_next_token(parser->lexer);

    if (parser->current.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "error: expected struct tag name after 'struct'\n");
        exit(1);
    }
    char tag[256];
    strncpy(tag, parser->current.value, sizeof(tag) - 1);
    tag[sizeof(tag) - 1] = '\0';
    parser->current = lexer_next_token(parser->lexer);

    StructTag *st = parser_register_struct_tag(parser, tag);

    if (parser->current.type == TOKEN_LBRACE) {
        parser->current = lexer_next_token(parser->lexer);

        int current_offset = 0;
        int max_align = 1;

        /* Stage 31: collect field descriptors while parsing. */
        StructField tmp_fields[64];
        int n_fields = 0;

        while (parser->current.type != TOKEN_RBRACE) {
            /* Parse field type specifier. */
            Type *field_base = parse_type_specifier(parser, NULL);

            /* Parse one or more declarators: each names a field. */
            do {
                if (parser->current.type == TOKEN_COMMA)
                    parser->current = lexer_next_token(parser->lexer);

                ParsedDeclarator d = parse_declarator(parser);
                /* Build the full field type from base + pointer stars. */
                Type *field_type = field_base;
                for (int i = 0; i < d.pointer_count; i++)
                    field_type = type_pointer(field_type);

                /* Stage 37: a non-pointer field of an incomplete struct type
                 * is invalid; pointer-to-incomplete is allowed. */
                if (field_type->kind == TYPE_STRUCT && field_type->size == 0) {
                    fprintf(stderr,
                            "error: field '%s' has incomplete struct type\n", d.name);
                    exit(1);
                }

                int fsz   = type_size(field_type);
                int falign = type_alignment(field_type);
                if (falign < 1) falign = 1;
                if (fsz < 1)    fsz = 1;

                if (falign > max_align) max_align = falign;
                /* Advance offset to satisfy field alignment. */
                current_offset = (current_offset + falign - 1) & ~(falign - 1);

                if (n_fields < 64) {
                    strncpy(tmp_fields[n_fields].name, d.name,
                            sizeof(tmp_fields[n_fields].name) - 1);
                    tmp_fields[n_fields].name[sizeof(tmp_fields[n_fields].name) - 1] = '\0';
                    tmp_fields[n_fields].offset    = current_offset;
                    tmp_fields[n_fields].kind      = field_type->kind;
                    tmp_fields[n_fields].full_type = (field_type->kind == TYPE_POINTER ||
                                                     field_type->kind == TYPE_ARRAY   ||
                                                     field_type->kind == TYPE_STRUCT)
                                                     ? field_type : NULL;
                    n_fields++;
                }
                current_offset += fsz;

            } while (parser->current.type == TOKEN_COMMA);

            parser_expect(parser, TOKEN_SEMICOLON);
        }
        parser_expect(parser, TOKEN_RBRACE);

        /* Round total size up to the struct's alignment. */
        int total_size = (current_offset + max_align - 1) & ~(max_align - 1);
        if (total_size == 0) total_size = 1; /* empty struct: 1 byte by convention */

        /* Stage 37: if a placeholder was created by a forward declaration,
         * update it in-place so any existing Type* references (e.g. typedef
         * entries) automatically reflect the completed layout. */
        if (st->type && st->type->size == 0) {
            st->type->size      = total_size;
            st->type->alignment = max_align;
        } else {
            st->type = type_struct(total_size, max_align);
        }

        /* Stage 31: copy collected fields into the Type. */
        if (n_fields > 0) {
            st->type->fields = calloc(n_fields, sizeof(StructField));
            memcpy(st->type->fields, tmp_fields, n_fields * sizeof(StructField));
            st->type->field_count = n_fields;
        }

    } else {
        /* Reference without body: "struct Tag" used as a type (e.g. pointer
         * target or forward declaration).  Stage 37: create an incomplete
         * placeholder so forward declarations and opaque pointer fields work. */
        if (!st->type)
            st->type = type_struct(0, 0);
    }

    return st->type;
}

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

    char tag[256] = "";
    int has_tag = 0;

    if (parser->current.type == TOKEN_IDENTIFIER) {
        strncpy(tag, parser->current.value, sizeof(tag) - 1);
        tag[sizeof(tag) - 1] = '\0';
        has_tag = 1;
        parser->current = lexer_next_token(parser->lexer);
    }

    if (parser->current.type == TOKEN_LBRACE) {
        parser->current = lexer_next_token(parser->lexer);

        if (has_tag) {
            /* Register or update the tag as defined. */
            EnumTag *et = NULL;
            for (int i = 0; i < parser->enum_tag_count; i++) {
                if (strcmp(parser->enum_tags[i].tag, tag) == 0) {
                    et = &parser->enum_tags[i];
                    break;
                }
            }
            if (!et) {
                if (parser->enum_tag_count >= PARSER_MAX_ENUM_TAGS) {
                    fprintf(stderr, "error: too many enum tags (max %d)\n",
                            PARSER_MAX_ENUM_TAGS);
                    exit(1);
                }
                et = &parser->enum_tags[parser->enum_tag_count++];
                strncpy(et->tag, tag, sizeof(et->tag) - 1);
                et->tag[sizeof(et->tag) - 1] = '\0';
            }
            et->is_defined = 1;
        }

        long next_val = 0;

        while (parser->current.type != TOKEN_RBRACE) {
            if (parser->current.type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "error: expected identifier in enumerator list, got '%s'\n",
                        parser->current.value);
                exit(1);
            }
            char ename[256];
            strncpy(ename, parser->current.value, sizeof(ename) - 1);
            ename[sizeof(ename) - 1] = '\0';
            parser->current = lexer_next_token(parser->lexer);

            for (int i = 0; i < parser->enum_const_count; i++) {
                if (strcmp(parser->enum_consts[i].name, ename) == 0) {
                    fprintf(stderr, "error: duplicate enumerator '%s'\n", ename);
                    exit(1);
                }
            }

            if (parser->current.type == TOKEN_ASSIGN) {
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type == TOKEN_INT_LITERAL) {
                    next_val = parser->current.long_value;
                    parser->current = lexer_next_token(parser->lexer);
                } else if (parser->current.type == TOKEN_CHAR_LITERAL) {
                    next_val = (long)(unsigned char)parser->current.value[0];
                    parser->current = lexer_next_token(parser->lexer);
                } else {
                    fprintf(stderr,
                            "error: enumerator value must be an integer or character literal\n");
                    exit(1);
                }
                /* Reject expressions: only a bare literal is accepted. */
                if (parser->current.type != TOKEN_COMMA &&
                    parser->current.type != TOKEN_RBRACE) {
                    fprintf(stderr,
                            "error: enumerator value must be an integer or character literal\n");
                    exit(1);
                }
            }

            if (parser->enum_const_count >= PARSER_MAX_ENUM_CONSTS) {
                fprintf(stderr, "error: too many enum constants (max %d)\n",
                        PARSER_MAX_ENUM_CONSTS);
                exit(1);
            }
            EnumConst *ec = &parser->enum_consts[parser->enum_const_count++];
            strncpy(ec->name, ename, sizeof(ec->name) - 1);
            ec->name[sizeof(ec->name) - 1] = '\0';
            ec->value = next_val++;

            if (parser->current.type == TOKEN_COMMA) {
                parser->current = lexer_next_token(parser->lexer);
                /* trailing comma allowed — if next is '}' the while exits */
            }
        }
        parser_expect(parser, TOKEN_RBRACE);

    } else {
        /* Enum reference without body: "enum Tag" */
        if (!has_tag) {
            fprintf(stderr, "error: expected identifier or '{' after 'enum'\n");
            exit(1);
        }
        int found = 0;
        for (int i = 0; i < parser->enum_tag_count; i++) {
            if (strcmp(parser->enum_tags[i].tag, tag) == 0 &&
                parser->enum_tags[i].is_defined) {
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "error: 'enum %s' is not defined\n", tag);
            exit(1);
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
    case TOKEN_VOID:  kind = TYPE_VOID;  t = type_void();  break;
    case TOKEN_CHAR:  kind = TYPE_CHAR;  t = type_char();  break;
    case TOKEN_SHORT: kind = TYPE_SHORT; t = type_short(); break;
    case TOKEN_LONG:  kind = TYPE_LONG;  t = type_long();  break;
    case TOKEN_INT:   kind = TYPE_INT;   t = type_int();   break;
    case TOKEN_UNSIGNED: {
        /* Stage 40: "unsigned" [ "char" | "short" | "int" | "long" ] */
        parser->current = lexer_next_token(parser->lexer); /* consume 'unsigned' */
        switch (parser->current.type) {
        case TOKEN_CHAR:
            kind = TYPE_CHAR;  t = type_unsigned_char();
            break;
        case TOKEN_SHORT:
            kind = TYPE_SHORT; t = type_unsigned_short();
            break;
        case TOKEN_INT:
            kind = TYPE_INT;   t = type_unsigned_int();
            break;
        case TOKEN_LONG:
            kind = TYPE_LONG;  t = type_unsigned_long();
            break;
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
            fprintf(stderr, "error: unknown type name '%s'\n",
                    parser->current.value);
            exit(1);
        }
        kind = entry->kind;
        if (entry->full_type) {
            t = entry->full_type; /* pointer typedef or unsigned scalar typedef */
        } else {
            switch (kind) {
            case TYPE_CHAR:  t = type_char();  break;
            case TYPE_SHORT: t = type_short(); break;
            case TYPE_LONG:  t = type_long();  break;
            default:         t = type_int();   break;
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
    default:
        fprintf(stderr, "error: expected integer type, got '%s'\n",
                parser->current.value);
        exit(1);
    }
    parser->current = lexer_next_token(parser->lexer);
    if (out_kind) *out_kind = kind;
    return t;
}

/*
 * <type_name> ::= <type_specifier> { "*" }
 *
 * Nameless type context: cast expressions and sizeof(type).
 * Returns the fully pointer-wrapped Type*.
 */
static Type *parse_type_name(Parser *parser) {
    /* Stage 39: consume optional leading const qualifier. */
    if (parser->current.type == TOKEN_CONST)
        parser->current = lexer_next_token(parser->lexer);
    /* Stage 40: consume optional leading unsigned qualifier (handled inside
     * parse_type_specifier when TOKEN_UNSIGNED is the current token). */
    Type *t = parse_type_specifier(parser, NULL);
    while (parser->current.type == TOKEN_STAR) {
        t = type_pointer(t);
        parser->current = lexer_next_token(parser->lexer);
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
    while (parser->current.type == TOKEN_STAR) {
        outer_stars++;
        parser->current = lexer_next_token(parser->lexer);
    }

    if (parser->current.type == TOKEN_LPAREN) {
        /* Parenthesized declarator: "(" { "*" } identifier [ suffix ] ")" */
        parser->current = lexer_next_token(parser->lexer); /* consume "(" */
        int inner_stars = 0;
        while (parser->current.type == TOKEN_STAR) {
            inner_stars++;
            parser->current = lexer_next_token(parser->lexer);
        }
        Token name = parser_expect(parser, TOKEN_IDENTIFIER);
        strncpy(d.name, name.value, sizeof(d.name) - 1);
        d.name[sizeof(d.name) - 1] = '\0';
        /* A function suffix on the inner identifier means "function returning
         * function pointer" (e.g. int (*fp())(int)) — not yet supported. */
        if (parser->current.type == TOKEN_LPAREN) {
            fprintf(stderr,
                    "error: functions returning function pointers are not supported\n");
            exit(1);
        }
        /* Optional array suffix inside parens: (*a[10]) */
        if (parser->current.type == TOKEN_LBRACKET) {
            d.is_array = 1;
            parser->current = lexer_next_token(parser->lexer);
            if (parser->current.type == TOKEN_INT_LITERAL) {
                Token size_tok = parser->current;
                parser->current = lexer_next_token(parser->lexer);
                int length = (int)size_tok.long_value;
                if (length <= 0) {
                    fprintf(stderr, "error: array size must be greater than zero\n");
                    exit(1);
                }
                d.array_length = length;
                d.has_size = 1;
            } else if (parser->current.type != TOKEN_RBRACKET) {
                fprintf(stderr, "error: array size must be an integer literal\n");
                exit(1);
            }
            parser_expect(parser, TOKEN_RBRACKET);
        }
        parser_expect(parser, TOKEN_RPAREN);
        /* Check suffix after the closing ")" */
        if (parser->current.type == TOKEN_LBRACKET) {
            fprintf(stderr, "error: pointer to array types are not supported\n");
            exit(1);
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
                if (parser->current.type != TOKEN_RPAREN) {
                    while (1) {
                        if (count >= FUNC_TYPE_MAX_PARAMS) {
                            fprintf(stderr,
                                    "error: too many parameters in function pointer"
                                    " type (max %d)\n", FUNC_TYPE_MAX_PARAMS);
                            exit(1);
                        }
                        /* Stage 39: consume optional const qualifier. */
                        if (parser->current.type == TOKEN_CONST)
                            parser->current = lexer_next_token(parser->lexer);
                        Type *pt = parse_type_specifier(parser, NULL);
                        int stars = 0;
                        while (parser->current.type == TOKEN_STAR) {
                            stars++;
                            parser->current = lexer_next_token(parser->lexer);
                        }
                        /* Optional parameter name — consume and discard. */
                        if (parser->current.type == TOKEN_IDENTIFIER)
                            parser->current = lexer_next_token(parser->lexer);
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
    strncpy(d.name, name.value, sizeof(d.name) - 1);
    d.name[sizeof(d.name) - 1] = '\0';
    if (parser->current.type == TOKEN_LBRACKET) {
        d.is_array = 1;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_INT_LITERAL) {
            Token size_tok = parser->current;
            parser->current = lexer_next_token(parser->lexer);
            int length = (int)size_tok.long_value;
            if (length <= 0) {
                fprintf(stderr, "error: array size must be greater than zero\n");
                exit(1);
            }
            d.array_length = length;
            d.has_size = 1;
        } else if (parser->current.type != TOKEN_RBRACKET) {
            fprintf(stderr, "error: array size must be an integer literal\n");
            exit(1);
        }
        parser_expect(parser, TOKEN_RBRACKET);
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
        ASTNode *node = ast_new(AST_INT_LITERAL, token.value);
        node->decl_type = token.literal_type;
        node->is_unsigned = token.is_unsigned;
        return node;
    }
    /* Stage 14-02: a string literal is a primary expression whose
     * logical type is char[N+1], where N is the literal's byte
     * length and the trailing slot holds the implicit NUL. The
     * payload bytes ride on node->value (copied below) and the
     * length is preserved on full_type->length.
     *
     * Stage 14-05: the decoded payload may contain embedded NUL
     * bytes (from `\0`), so the value is copied with memcpy bounded
     * by token.length rather than via ast_new's strncpy, and the
     * count is also stashed on byte_length for downstream consumers
     * (notably codegen, where full_type is rewritten to `char *`
     * during the array-to-pointer decay and the length on full_type
     * is no longer reachable). */
    if (parser->current.type == TOKEN_STRING_LITERAL) {
        Token token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *node = ast_new(AST_STRING_LITERAL, NULL);
        memcpy(node->value, token.value, token.length);
        node->value[token.length] = '\0';
        node->byte_length = token.length;
        node->decl_type = TYPE_ARRAY;
        node->full_type = type_array(type_char(), token.length + 1);
        return node;
    }
    /* Stage 15-02: a character literal is a primary expression of type
     * int. The token already carries the decoded byte at value[0] and
     * the evaluated integer at long_value; mirror the string-literal
     * convention by storing the decoded byte at node->value[0]. The
     * integer value used by codegen is recovered as
     * (unsigned char)node->value[0]. */
    if (parser->current.type == TOKEN_CHAR_LITERAL) {
        Token token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *node = ast_new(AST_CHAR_LITERAL, NULL);
        node->value[0] = token.value[0];
        node->value[1] = '\0';
        node->byte_length = 1;
        node->decl_type = TYPE_INT;
        return node;
    }
    if (parser->current.type == TOKEN_IDENTIFIER) {
        /* Stage 29: fold enum constants to integer literals before any other
         * identifier resolution. */
        for (int i = 0; i < parser->enum_const_count; i++) {
            if (strcmp(parser->enum_consts[i].name, parser->current.value) == 0) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%ld", parser->enum_consts[i].value);
                parser->current = lexer_next_token(parser->lexer);
                ASTNode *node = ast_new(AST_INT_LITERAL, buf);
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
                ASTNode *call = ast_new(AST_FUNCTION_CALL, token.value);
                if (parser->current.type != TOKEN_RPAREN) {
                    ast_add_child(call, parse_assignment_expression(parser));
                    while (parser->current.type == TOKEN_COMMA) {
                        parser->current = lexer_next_token(parser->lexer);
                        ast_add_child(call, parse_assignment_expression(parser));
                    }
                }
                parser_expect(parser, TOKEN_RPAREN);
                if (sig->param_count != call->child_count) {
                    fprintf(stderr,
                            "error: function '%s' expects %d arguments, got %d\n",
                            token.value, sig->param_count, call->child_count);
                    exit(1);
                }
                if (call->child_count > 6) {
                    fprintf(stderr,
                            "error: function '%s' call has %d arguments; max supported is 6\n",
                            token.value, call->child_count);
                    exit(1);
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
                ASTNode *callee = ast_new(AST_VAR_REF, token.value);
                ASTNode *call = ast_new(AST_INDIRECT_CALL, NULL);
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
        return ast_new(AST_VAR_REF, token.value);
    }
    if (parser->current.type == TOKEN_LPAREN) {
        parser_expect(parser, TOKEN_LPAREN);
        ASTNode *expr = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        return expr;
    }
    fprintf(stderr, "error: expected expression, got '%s'\n",
            parser->current.value);
    exit(1);
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
static ASTNode *parse_postfix(Parser *parser) {
    ASTNode *expr = parse_primary(parser);
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
            ASTNode *call = ast_new(AST_INDIRECT_CALL, NULL);
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
             * pointer-array element subscripts like names[0][1] work. */
            if (expr->type != AST_VAR_REF && expr->type != AST_DEREF &&
                expr->type != AST_ARRAY_INDEX) {
                fprintf(stderr, "error: subscript base must be an identifier\n");
                exit(1);
            }
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *index = parse_expression(parser);
            parser_expect(parser, TOKEN_RBRACKET);
            ASTNode *node = ast_new(AST_ARRAY_INDEX, NULL);
            ast_add_child(node, expr);
            ast_add_child(node, index);
            expr = node;
            continue;
        }
        if (parser->current.type == TOKEN_DOT) {
            parser->current = lexer_next_token(parser->lexer);
            Token field = parser_expect(parser, TOKEN_IDENTIFIER);
            ASTNode *node = ast_new(AST_MEMBER_ACCESS, field.value);
            ast_add_child(node, expr);
            expr = node;
            continue;
        }
        if (parser->current.type == TOKEN_ARROW) {
            parser->current = lexer_next_token(parser->lexer);
            Token field = parser_expect(parser, TOKEN_IDENTIFIER);
            ASTNode *node = ast_new(AST_ARROW_ACCESS, field.value);
            ast_add_child(node, expr);
            expr = node;
            continue;
        }
        if (expr->type != AST_VAR_REF) {
            fprintf(stderr, "error: postfix %s requires an identifier\n",
                    parser->current.value);
            exit(1);
        }
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *node = ast_new(AST_POSTFIX_INC_DEC, op.value);
        ast_add_child(node, expr);
        expr = node;
    }
    return expr;
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
            ASTNode *node = ast_new(AST_SIZEOF_EXPR, NULL);
            ast_add_child(node, operand);
            return node;
        }
        /* Peek past '(' to distinguish sizeof(type) from sizeof(expression) */
        parser->current = lexer_next_token(parser->lexer); /* consume '(' */
        if (parser->current.type == TOKEN_VOID) {
            fprintf(stderr, "error: sizeof applied to void type\n");
            exit(1);
        }
        if (parser->current.type == TOKEN_CHAR ||
            parser->current.type == TOKEN_SHORT ||
            parser->current.type == TOKEN_INT ||
            parser->current.type == TOKEN_LONG ||
            parser->current.type == TOKEN_UNSIGNED ||
            parser->current.type == TOKEN_STRUCT ||
            (parser->current.type == TOKEN_IDENTIFIER &&
             parser_find_typedef(parser, parser->current.value))) {
            /* <sizeof_expression> ::= "sizeof" "(" <type_name> ")" */
            Type *t = parse_type_name(parser);
            parser_expect(parser, TOKEN_RPAREN);
            ASTNode *node = ast_new(AST_SIZEOF_TYPE, NULL);
            node->decl_type = t->kind;
            if (t->kind == TYPE_STRUCT)
                node->full_type = t;
            return node;
        }
        if (parser->current.type == TOKEN_RPAREN) {
            fprintf(stderr, "error: expected expression or type in sizeof\n");
            exit(1);
        }
        /* sizeof(<expression>) */
        ASTNode *operand = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        ASTNode *node = ast_new(AST_SIZEOF_EXPR, NULL);
        ast_add_child(node, operand);
        return node;
    }
    if (parser->current.type == TOKEN_INCREMENT ||
        parser->current.type == TOKEN_DECREMENT) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *operand = parse_unary(parser);
        if (operand->type != AST_VAR_REF) {
            fprintf(stderr, "error: prefix %s requires an identifier\n", op.value);
            exit(1);
        }
        ASTNode *node = ast_new(AST_PREFIX_INC_DEC, op.value);
        ast_add_child(node, operand);
        return node;
    }
    if (parser->current.type == TOKEN_AMPERSAND) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *operand = parse_unary(parser);
        /* Stage 13-04: address-of also accepts an array subscript so
         * `&a[i]` produces a pointer to the i-th element. */
        if (operand->type != AST_VAR_REF &&
            operand->type != AST_ARRAY_INDEX) {
            fprintf(stderr, "error: address-of requires an lvalue\n");
            exit(1);
        }
        ASTNode *node = ast_new(AST_ADDR_OF, NULL);
        ast_add_child(node, operand);
        return node;
    }
    if (parser->current.type == TOKEN_STAR) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *operand = parse_unary(parser);
        ASTNode *node = ast_new(AST_DEREF, NULL);
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
        ASTNode *unary = ast_new(AST_UNARY_OP, op.value);
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
static ASTNode *parse_cast(Parser *parser) {
    if (parser->current.type == TOKEN_LPAREN) {
        int saved_pos = parser->lexer->pos;
        Token saved_token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_VOID ||
            parser->current.type == TOKEN_CHAR ||
            parser->current.type == TOKEN_SHORT ||
            parser->current.type == TOKEN_INT ||
            parser->current.type == TOKEN_LONG ||
            parser->current.type == TOKEN_UNSIGNED ||
            (parser->current.type == TOKEN_IDENTIFIER &&
             parser_find_typedef(parser, parser->current.value))) {
            Type *cast_type = parse_type_name(parser);
            parser_expect(parser, TOKEN_RPAREN);
            ASTNode *operand = parse_cast(parser);
            ASTNode *cast = ast_new(AST_CAST, NULL);
            cast->decl_type = cast_type->kind;
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
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
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
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
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
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
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
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
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
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
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
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
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
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
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
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
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
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
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
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
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
        fprintf(stderr, "error: expected expression after '?'\n");
        exit(1);
    }
    ASTNode *true_expr = parse_expression(parser);
    if (parser->current.type != TOKEN_COLON) {
        fprintf(stderr, "error: expected ':' in conditional expression\n");
        exit(1);
    }
    parser->current = lexer_next_token(parser->lexer); /* consume ':' */
    ASTNode *false_expr = parse_conditional(parser);
    ASTNode *node = ast_new(AST_CONDITIONAL_EXPR, NULL);
    ast_add_child(node, cond);
    ast_add_child(node, true_expr);
    ast_add_child(node, false_expr);
    return node;
}

/*
 * Stage 32: <initializer> ::= <assignment_expression>
 *                            | "{" <initializer_list> [ "," ] "}"
 *
 * Returns an AST_INITIALIZER_LIST node when a brace-initializer is
 * parsed; otherwise returns the result of parse_assignment_expression.
 */
static ASTNode *parse_initializer(Parser *parser) {
    if (parser->current.type != TOKEN_LBRACE)
        return parse_assignment_expression(parser);

    /* Consume "{". */
    parser->current = lexer_next_token(parser->lexer);

    ASTNode *list = ast_new(AST_INITIALIZER_LIST, NULL);

    /* Empty brace-initializer "{}" — zero-fill everything. */
    if (parser->current.type == TOKEN_RBRACE) {
        parser->current = lexer_next_token(parser->lexer);
        return list;
    }

    ast_add_child(list, parse_initializer(parser));
    while (parser->current.type == TOKEN_COMMA) {
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_RBRACE)
            break; /* trailing comma */
        ast_add_child(list, parse_initializer(parser));
    }
    if (parser->current.type != TOKEN_RBRACE) {
        fprintf(stderr, "error: expected '}' to close initializer list\n");
        exit(1);
    }
    parser->current = lexer_next_token(parser->lexer);
    return list;
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
            ASTNode *assign = ast_new(AST_ASSIGNMENT, saved_token.value);
            if (op.type != TOKEN_ASSIGN) {
                /* a op= b  =>  a = a op b */
                ASTNode *var_ref = ast_new(AST_VAR_REF, saved_token.value);
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
                ASTNode *binop = ast_new(AST_BINARY_OP, bin_op);
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
    if (parser->current.type == TOKEN_ASSIGN) {
        if (lhs->type != AST_DEREF && lhs->type != AST_VAR_REF &&
            lhs->type != AST_ARRAY_INDEX &&
            lhs->type != AST_MEMBER_ACCESS &&
            lhs->type != AST_ARROW_ACCESS) {
            fprintf(stderr, "error: assignment target must be an lvalue\n");
            exit(1);
        }
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *rhs = parse_assignment_expression(parser);
        ASTNode *assign = ast_new(AST_ASSIGNMENT, NULL);
        ast_add_child(assign, lhs);
        ast_add_child(assign, rhs);
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
        ASTNode *comma = ast_new(AST_COMMA_EXPR, ",");
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
    ASTNode *block = ast_new(AST_BLOCK, NULL);
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

    ASTNode *if_node = ast_new(AST_IF_STATEMENT, NULL);
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

    ASTNode *while_node = ast_new(AST_WHILE_STATEMENT, NULL);
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

    ASTNode *do_while_node = ast_new(AST_DO_WHILE_STATEMENT, NULL);
    ast_add_child(do_while_node, body);
    ast_add_child(do_while_node, condition);

    return do_while_node;
}

/*
 * <for_statement> ::= "for" "(" [<expression>] ";" [<expression>] ";" [<expression>] ")" <statement>
 *
 * Children layout: [init, condition, update, body]
 * Any of init/condition/update may be NULL when omitted.
 */
static ASTNode *parse_for_statement(Parser *parser) {
    parser_expect(parser, TOKEN_FOR);
    parser_expect(parser, TOKEN_LPAREN);

    ASTNode *for_node = ast_new(AST_FOR_STATEMENT, NULL);

    /* init */
    ASTNode *init = NULL;
    if (parser->current.type != TOKEN_SEMICOLON) {
        init = parse_expression(parser);
    }
    parser_expect(parser, TOKEN_SEMICOLON);

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

    /* Store as children — NULLs are stored directly */
    for_node->children[0] = init;
    for_node->children[1] = condition;
    for_node->children[2] = update;
    for_node->children[3] = body;
    for_node->child_count = 4;

    return for_node;
}

/*
 * <switch_statement>   ::= "switch" "(" <expression> ")" <statement>
 * <labeled_statement>  ::= "case" <integer_literal> ":" <statement>
 *                        | "default" ":" <statement>
 *
 * Stage 10-03-03: the switch body is any statement. `case` and
 * `default` labels are parsed as labeled statements inside
 * parse_statement and are only legal while switch_depth > 0.
 */
static ASTNode *parse_switch_statement(Parser *parser) {
    parser_expect(parser, TOKEN_SWITCH);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);

    ASTNode *switch_node = ast_new(AST_SWITCH_STATEMENT, NULL);
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
    /* Stage 23: storage class specifiers are not supported at block scope. */
    if (parser->current.type == TOKEN_EXTERN ||
        parser->current.type == TOKEN_STATIC) {
        fprintf(stderr,
                "error: storage class specifier not allowed in block scope\n");
        exit(1);
    }
    /* Stage 28-01/28-02/28-03/28-04: typedef declaration at block scope. */
    if (parser->current.type == TOKEN_TYPEDEF) {
        parser->current = lexer_next_token(parser->lexer);
        TypeKind base_kind;
        Type *base_type = parse_type_specifier(parser, &base_kind);
        ParsedDeclarator d = parse_declarator(parser);
        if (d.is_function) {
            fprintf(stderr,
                    "error: only scalar, pointer, and array typedefs are supported\n");
            exit(1);
        }
        /* Stage 28-04: array typedef — register with the full array Type*. */
        if (d.is_array) {
            if (!d.has_size) {
                fprintf(stderr, "error: array typedef requires explicit size\n");
                exit(1);
            }
            parser_expect(parser, TOKEN_SEMICOLON);
            Type *array_type = type_array(base_type, d.array_length);
            parser_register_typedef(parser, d.name, TYPE_ARRAY, array_type);
            return ast_new(AST_TYPEDEF_DECL, d.name);
        }
        if (parser->current.type == TOKEN_ASSIGN) {
            fprintf(stderr,
                    "error: typedef declaration cannot have an initializer\n");
            exit(1);
        }
        parser_expect(parser, TOKEN_SEMICOLON);
        if (d.is_func_pointer) {
            Type *fp_type = build_fp_type(base_type, &d);
            parser_register_typedef(parser, d.name, TYPE_POINTER, fp_type);
            return ast_new(AST_TYPEDEF_DECL, d.name);
        }
        Type *full_type = base_type;
        for (int i = 0; i < d.pointer_count; i++)
            full_type = type_pointer(full_type);
        TypeKind typedef_kind = (d.pointer_count > 0 || base_kind == TYPE_POINTER)
                                ? TYPE_POINTER : base_kind;
        /* Stage 40: for unsigned scalar typedefs, store the base Type* (is_signed=0)
         * as full_type so the signedness is preserved when the typedef is resolved. */
        Type *reg_full_type = (typedef_kind == TYPE_POINTER || typedef_kind == TYPE_STRUCT)
                              ? full_type
                              : (!base_type->is_signed ? base_type : NULL);
        parser_register_typedef(parser, d.name, typedef_kind, reg_full_type);
        return ast_new(AST_TYPEDEF_DECL, d.name);
    }
    /* labeled_statement: <identifier> ":" <statement> */
    if (parser->current.type == TOKEN_IDENTIFIER) {
        int saved_pos = parser->lexer->pos;
        Token saved_token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_COLON) {
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *node = ast_new(AST_LABEL_STATEMENT, saved_token.value);
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
     * Stage 39: optional leading const qualifier. */
    int local_is_const = 0;
    if (parser->current.type == TOKEN_CONST) {
        local_is_const = 1;
        parser->current = lexer_next_token(parser->lexer);
    }
    if (local_is_const ||
        parser->current.type == TOKEN_VOID ||
        parser->current.type == TOKEN_CHAR ||
        parser->current.type == TOKEN_SHORT ||
        parser->current.type == TOKEN_INT ||
        parser->current.type == TOKEN_LONG ||
        parser->current.type == TOKEN_UNSIGNED ||
        parser->current.type == TOKEN_ENUM ||
        parser->current.type == TOKEN_STRUCT ||
        (parser->current.type == TOKEN_IDENTIFIER &&
         parser_find_typedef(parser, parser->current.value))) {
        TypeKind base_kind;
        Type *base_type = parse_type_specifier(parser, &base_kind);

        /* Standalone type declaration with no variable (e.g. "struct S{};"). */
        if (parser->current.type == TOKEN_SEMICOLON) {
            parser_expect(parser, TOKEN_SEMICOLON);
            return ast_new(AST_TYPEDEF_DECL, "");
        }

        ParsedDeclarator d = parse_declarator(parser);

        /* Reject `void x;` — void cannot be an object type. */
        if (base_kind == TYPE_VOID && d.pointer_count == 0 &&
            !d.is_func_pointer && !d.is_array) {
            fprintf(stderr,
                    "error: cannot declare variable '%s' of type void\n", d.name);
            exit(1);
        }

        if (d.is_func_pointer) {
            ASTNode *decl = ast_new(AST_DECLARATION, d.name);
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

        /* Build the fully-wrapped element type: base + pointer levels. */
        Type *full_type = base_type;
        for (int i = 0; i < d.pointer_count; i++) {
            full_type = type_pointer(full_type);
        }

        ASTNode *decl = ast_new(AST_DECLARATION, d.name);
        /* Stage 39: const applies to the variable when no pointer depth. */
        decl->is_const = (local_is_const && d.pointer_count == 0 && !d.is_array) ? 1 : 0;

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
                        fprintf(stderr,
                                "error: string initializer only supported for char arrays\n");
                        exit(1);
                    }
                    Token str_tok = parser->current;
                    parser->current = lexer_next_token(parser->lexer);
                    ASTNode *str_init = ast_new(AST_STRING_LITERAL, NULL);
                    memcpy(str_init->value, str_tok.value, str_tok.length);
                    str_init->value[str_tok.length] = '\0';
                    str_init->byte_length = str_tok.length;
                    str_init->decl_type = TYPE_ARRAY;
                    str_init->full_type = type_array(type_char(), str_tok.length + 1);
                    int needed = str_init->byte_length + 1;
                    if (has_size) {
                        if (length < needed) {
                            fprintf(stderr,
                                    "error: array too small for string literal initializer\n");
                            exit(1);
                        }
                    } else {
                        length = needed;
                    }
                    init_node = str_init;
                } else {
                    if (!has_size) {
                        fprintf(stderr,
                                "error: omitted array size requires string literal initializer\n");
                    } else {
                        fprintf(stderr,
                                "error: array initializer must be a brace-enclosed list or string literal\n");
                    }
                    exit(1);
                }
            } else if (!has_size) {
                fprintf(stderr,
                        "error: array size required unless initialized from string literal\n");
                exit(1);
            }

            Type *array_type = type_array(full_type, length);
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
        } else {
            decl->decl_type = base_kind;
            /* Stage 40: mark unsigned scalar. */
            decl->is_unsigned = !base_type->is_signed;
        }
        if (parser->current.type == TOKEN_ASSIGN) {
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *init = parse_initializer(parser); /* stage 32: allows brace lists */
            if (init->type == AST_INITIALIZER_LIST &&
                decl->decl_type != TYPE_STRUCT && decl->decl_type != TYPE_ARRAY) {
                fprintf(stderr, "error: brace initializer not valid for scalar type\n");
                exit(1);
            }
            ast_add_child(decl, init);
        }
        if (parser->current.type != TOKEN_COMMA) {
            parser_expect(parser, TOKEN_SEMICOLON);
            return decl;
        }
        /* <init_declarator_list>: one or more declarators sharing the same base type. */
        ASTNode *list = ast_new(AST_DECL_LIST, NULL);
        ast_add_child(list, decl);
        while (parser->current.type == TOKEN_COMMA) {
            parser->current = lexer_next_token(parser->lexer);
            ParsedDeclarator d2 = parse_declarator(parser);
            if (d2.is_array) {
                fprintf(stderr, "error: array declarator in multi-declarator list not supported\n");
                exit(1);
            }
            Type *full_type2 = base_type;
            for (int i = 0; i < d2.pointer_count; i++) {
                full_type2 = type_pointer(full_type2);
            }
            ASTNode *next_decl = ast_new(AST_DECLARATION, d2.name);
            next_decl->is_const = (local_is_const && d2.pointer_count == 0) ? 1 : 0;
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
        ASTNode *stmt = ast_new(AST_RETURN_STATEMENT, NULL);
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
            fprintf(stderr, "error: 'case' label outside of switch\n");
            exit(1);
        }
        parser->current = lexer_next_token(parser->lexer);
        Token value = parser_expect(parser, TOKEN_INT_LITERAL);
        parser_expect(parser, TOKEN_COLON);
        ASTNode *node = ast_new(AST_CASE_SECTION, NULL);
        ast_add_child(node, ast_new(AST_INT_LITERAL, value.value));
        ast_add_child(node, parse_statement(parser));
        return node;
    }
    if (parser->current.type == TOKEN_DEFAULT) {
        if (parser->switch_depth == 0) {
            fprintf(stderr, "error: 'default' label outside of switch\n");
            exit(1);
        }
        parser->current = lexer_next_token(parser->lexer);
        parser_expect(parser, TOKEN_COLON);
        ASTNode *node = ast_new(AST_DEFAULT_SECTION, NULL);
        ast_add_child(node, parse_statement(parser));
        return node;
    }
    if (parser->current.type == TOKEN_LBRACE) {
        return parse_block(parser);
    }
    if (parser->current.type == TOKEN_BREAK) {
        if (parser->loop_depth == 0 && parser->switch_depth == 0) {
            fprintf(stderr, "error: break outside of loop or switch\n");
            exit(1);
        }
        parser->current = lexer_next_token(parser->lexer);
        parser_expect(parser, TOKEN_SEMICOLON);
        return ast_new(AST_BREAK_STATEMENT, NULL);
    }
    if (parser->current.type == TOKEN_CONTINUE) {
        if (parser->loop_depth == 0) {
            fprintf(stderr, "error: continue outside of loop\n");
            exit(1);
        }
        parser->current = lexer_next_token(parser->lexer);
        parser_expect(parser, TOKEN_SEMICOLON);
        return ast_new(AST_CONTINUE_STATEMENT, NULL);
    }
    if (parser->current.type == TOKEN_GOTO) {
        parser->current = lexer_next_token(parser->lexer);
        Token name = parser_expect(parser, TOKEN_IDENTIFIER);
        parser_expect(parser, TOKEN_SEMICOLON);
        return ast_new(AST_GOTO_STATEMENT, name.value);
    }
    /* expression_stmt (includes assignments, since assignment is now an expression) */
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);
    ASTNode *stmt = ast_new(AST_EXPRESSION_STMT, NULL);
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
    /* Stage 39: consume optional leading const qualifier on parameter types. */
    if (parser->current.type == TOKEN_CONST)
        parser->current = lexer_next_token(parser->lexer);
    TypeKind base_kind;
    Type *base_type = parse_type_specifier(parser, &base_kind);

    /* Optional declarator: absent when next token is "," or ")". */
    if (parser->current.type == TOKEN_COMMA ||
        parser->current.type == TOKEN_RPAREN) {
        ASTNode *param = ast_new(AST_PARAM, "");
        param->decl_type = base_kind;
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
    }
    if (leading_stars > 0 &&
        (parser->current.type == TOKEN_COMMA ||
         parser->current.type == TOKEN_RPAREN)) {
        ASTNode *param = ast_new(AST_PARAM, "");
        Type *full_type = base_type;
        for (int i = 0; i < leading_stars; i++)
            full_type = type_pointer(full_type);
        param->decl_type = TYPE_POINTER;
        param->full_type = full_type;
        return param;
    }

    ParsedDeclarator d = parse_declarator(parser);
    /* Any stars we pre-consumed contribute to the declarator's pointer
     * count. parse_declarator saw zero leading stars for this declarator. */
    d.pointer_count += leading_stars;
    ASTNode *param = ast_new(AST_PARAM, d.name);
    if (d.is_func_pointer) {
        param->decl_type = TYPE_POINTER;
        param->full_type = build_fp_type(base_type, &d);
        return param;
    }
    Type *full_type = base_type;
    for (int i = 0; i < d.pointer_count; i++) {
        full_type = type_pointer(full_type);
    }
    if (d.pointer_count > 0) {
        param->decl_type = TYPE_POINTER;
        param->full_type = full_type;
    } else {
        param->decl_type = base_kind;
        param->is_unsigned = !base_type->is_signed;
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
        ASTNode *next = parse_parameter_declaration(parser);
        if (next->value[0] != '\0') {
            for (int i = 0; i < func->child_count; i++) {
                if (func->children[i]->type == AST_PARAM &&
                    func->children[i]->value[0] != '\0' &&
                    strcmp(func->children[i]->value, next->value) == 0) {
                    fprintf(stderr,
                            "error: duplicate parameter '%s' in function '%s'\n",
                            next->value, func->value);
                    exit(1);
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
} DeclSpecResult;

static DeclSpecResult parse_declaration_specifiers(Parser *parser) {
    DeclSpecResult r;
    r.sc = SC_NONE;
    r.base_kind = TYPE_INT;
    r.base_type = NULL;
    r.is_const = 0;
    int has_sc = 0;
    int has_type = 0;

    while (1) {
        if (parser->current.type == TOKEN_CONST) {
            r.is_const = 1;
            parser->current = lexer_next_token(parser->lexer);
        } else if (parser->current.type == TOKEN_EXTERN ||
            parser->current.type == TOKEN_STATIC ||
            parser->current.type == TOKEN_TYPEDEF) {
            if (has_sc) {
                fprintf(stderr,
                        "error: multiple storage class specifiers are not allowed\n");
                exit(1);
            }
            has_sc = 1;
            if (parser->current.type == TOKEN_EXTERN) r.sc = SC_EXTERN;
            else if (parser->current.type == TOKEN_STATIC) r.sc = SC_STATIC;
            else r.sc = SC_TYPEDEF;
            parser->current = lexer_next_token(parser->lexer);
        } else if (parser->current.type == TOKEN_VOID ||
                   parser->current.type == TOKEN_CHAR ||
                   parser->current.type == TOKEN_SHORT ||
                   parser->current.type == TOKEN_INT ||
                   parser->current.type == TOKEN_LONG ||
                   parser->current.type == TOKEN_UNSIGNED ||
                   parser->current.type == TOKEN_ENUM ||
                   parser->current.type == TOKEN_STRUCT ||
                   (!has_type &&
                    parser->current.type == TOKEN_IDENTIFIER &&
                    parser_find_typedef(parser, parser->current.value))) {
            if (has_type) {
                fprintf(stderr,
                        "error: multiple type specifiers are not allowed\n");
                exit(1);
            }
            has_type = 1;
            r.base_type = parse_type_specifier(parser, &r.base_kind);
        } else {
            break;
        }
    }

    if (!has_type) {
        fprintf(stderr, "error: expected type specifier\n");
        exit(1);
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
        return ast_new(AST_TYPEDEF_DECL, "");
    }

    ParsedDeclarator d = parse_declarator(parser);

    /* Stage 28-01/28-02/28-03/28-04: typedef declaration at file scope. */
    if (sc == SC_TYPEDEF) {
        if (d.is_function) {
            fprintf(stderr,
                    "error: only scalar, pointer, and array typedefs are supported\n");
            exit(1);
        }
        /* Stage 28-04: array typedef — register with the full array Type*. */
        if (d.is_array) {
            if (!d.has_size) {
                fprintf(stderr, "error: array typedef requires explicit size\n");
                exit(1);
            }
            parser_expect(parser, TOKEN_SEMICOLON);
            Type *array_type = type_array(base_type, d.array_length);
            parser_register_typedef(parser, d.name, TYPE_ARRAY, array_type);
            return ast_new(AST_TYPEDEF_DECL, d.name);
        }
        if (parser->current.type == TOKEN_ASSIGN) {
            fprintf(stderr,
                    "error: typedef declaration cannot have an initializer\n");
            exit(1);
        }
        parser_expect(parser, TOKEN_SEMICOLON);
        if (d.is_func_pointer) {
            Type *fp_type = build_fp_type(base_type, &d);
            parser_register_typedef(parser, d.name, TYPE_POINTER, fp_type);
            return ast_new(AST_TYPEDEF_DECL, d.name);
        }
        Type *full_type = base_type;
        for (int i = 0; i < d.pointer_count; i++)
            full_type = type_pointer(full_type);
        TypeKind typedef_kind = (d.pointer_count > 0 || base_kind == TYPE_POINTER)
                                ? TYPE_POINTER : base_kind;
        /* Stage 40: for unsigned scalar typedefs, store the base Type* so
         * signedness is preserved when the typedef name is later resolved. */
        Type *reg_full_type = (typedef_kind == TYPE_POINTER || typedef_kind == TYPE_STRUCT)
                              ? full_type
                              : (!base_type->is_signed ? base_type : NULL);
        parser_register_typedef(parser, d.name, typedef_kind, reg_full_type);
        return ast_new(AST_TYPEDEF_DECL, d.name);
    }

    /* Stage 25-01/25-02: function-pointer file-scope declaration. */
    if (d.is_func_pointer) {
        Type *fp_type = build_fp_type(base_type, &d);
        if (sc == SC_EXTERN && parser->current.type == TOKEN_ASSIGN) {
            fprintf(stderr,
                    "error: extern declaration of '%s' cannot have an initializer\n",
                    d.name);
            exit(1);
        }
        if (parser_find_function(parser, d.name)) {
            fprintf(stderr,
                    "error: '%s' redeclared as a different kind of symbol\n", d.name);
            exit(1);
        }
        parser_register_global(parser, d.name, TYPE_POINTER, sc, fp_type);
        ASTNode *decl = ast_new(AST_DECLARATION, d.name);
        decl->storage_class = sc;
        decl->decl_type = TYPE_POINTER;
        decl->full_type = fp_type;
        /* Stage 25-02: optional function-designator initializer. */
        if (parser->current.type == TOKEN_ASSIGN) {
            parser->current = lexer_next_token(parser->lexer);
            if (parser->current.type != TOKEN_IDENTIFIER) {
                fprintf(stderr,
                        "error: function pointer initializer must be a function name\n");
                exit(1);
            }
            Token init_tok = parser->current;
            parser->current = lexer_next_token(parser->lexer);
            FuncSig *sig = parser_find_function(parser, init_tok.value);
            if (!sig) {
                fprintf(stderr,
                        "error: '%s' is not a declared function\n", init_tok.value);
                exit(1);
            }
            /* Verify compatibility against the declared function pointer type. */
            Type *fp_fn = fp_type->base; /* TYPE_FUNCTION node */
            if (fp_fn->base->kind != sig->return_type ||
                fp_fn->param_count != sig->param_count) {
                fprintf(stderr,
                        "error: incompatible function pointer type in initializer of '%s'\n",
                        d.name);
                exit(1);
            }
            for (int i = 0; i < sig->param_count; i++) {
                if (!fp_fn->params[i] ||
                    fp_fn->params[i]->kind != sig->param_types[i]) {
                    fprintf(stderr,
                            "error: incompatible function pointer type in initializer of '%s'\n",
                            d.name);
                    exit(1);
                }
            }
            ASTNode *init_node = ast_new(AST_VAR_REF, init_tok.value);
            ast_add_child(decl, init_node);
        }
        parser_expect(parser, TOKEN_SEMICOLON);
        return decl;
    }

    if (!d.is_function) {
        if (parser->current.type == TOKEN_LBRACE) {
            fprintf(stderr, "error: '%s' is not a function declarator\n", d.name);
            exit(1);
        }
        /* Reject `void g;` — void cannot be an object type. */
        if (base_kind == TYPE_VOID && d.pointer_count == 0 &&
            !d.is_func_pointer && !d.is_array) {
            fprintf(stderr,
                    "error: cannot declare variable '%s' of type void\n", d.name);
            exit(1);
        }
        Type *full_type = base_type;
        for (int i = 0; i < d.pointer_count; i++)
            full_type = type_pointer(full_type);

        /* Stage 23: extern with initializer is invalid. */
        if (sc == SC_EXTERN && parser->current.type == TOKEN_ASSIGN) {
            fprintf(stderr,
                    "error: extern declaration of '%s' cannot have an initializer\n",
                    d.name);
            exit(1);
        }

        /* Stage 22-02/23: detect function/object name conflicts and register. */
        if (parser_find_function(parser, d.name)) {
            fprintf(stderr,
                    "error: '%s' redeclared as a different kind of symbol\n", d.name);
            exit(1);
        }
        TypeKind obj_kind = d.is_array ? TYPE_ARRAY :
                            (d.pointer_count > 0 || base_kind == TYPE_POINTER) ? TYPE_POINTER : base_kind;
        Type *reg_full_type = (obj_kind == TYPE_POINTER || obj_kind == TYPE_STRUCT) ? full_type : NULL;
        parser_register_global(parser, d.name, obj_kind, sc, reg_full_type);

        ASTNode *decl = ast_new(AST_DECLARATION, d.name);
        decl->storage_class = sc;
        /* Stage 39: const applies to the variable when no pointer depth. */
        decl->is_const = (ds.is_const && d.pointer_count == 0 && !d.is_array) ? 1 : 0;
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
                        fprintf(stderr,
                                "error: too many initializers for array '%s'\n", d.name);
                        exit(1);
                    }
                } else if (parser->current.type == TOKEN_STRING_LITERAL) {
                    if (full_type->kind != TYPE_CHAR) {
                        fprintf(stderr,
                                "error: string initializer only supported for char arrays\n");
                        exit(1);
                    }
                    Token str_tok = parser->current;
                    parser->current = lexer_next_token(parser->lexer);
                    ASTNode *str_init = ast_new(AST_STRING_LITERAL, NULL);
                    memcpy(str_init->value, str_tok.value, str_tok.length);
                    str_init->value[str_tok.length] = '\0';
                    str_init->byte_length = str_tok.length;
                    int needed = str_init->byte_length + 1;
                    if (has_size) {
                        if (length < needed) {
                            fprintf(stderr,
                                    "error: array too small for string literal initializer\n");
                            exit(1);
                        }
                    } else {
                        length = needed;
                    }
                    init_node = str_init;
                } else {
                    fprintf(stderr,
                            "error: array initializer must be a brace-enclosed list or string literal\n");
                    exit(1);
                }
            } else if (!has_size) {
                fprintf(stderr,
                        "error: array size required for file-scope declaration '%s'\n",
                        d.name);
                exit(1);
            }

            decl->decl_type = TYPE_ARRAY;
            decl->full_type = type_array(full_type, length);
            if (init_node)
                ast_add_child(decl, init_node);
            parser_expect(parser, TOKEN_SEMICOLON);
            return decl;
        }
        if (d.pointer_count > 0 || base_kind == TYPE_POINTER) {
            decl->decl_type = TYPE_POINTER;
            decl->full_type = full_type;
        } else if (base_kind == TYPE_STRUCT) {
            decl->decl_type = TYPE_STRUCT;
            decl->full_type = full_type;
        } else {
            decl->decl_type = base_kind;
            /* Stage 40: mark unsigned scalar. */
            decl->is_unsigned = !base_type->is_signed;
        }
        /* Stage 22-02: optional constant initializer.
         * Stage 43: also accept string literals for pointer globals.
         * Stage 44: accept brace-list initializers for struct globals. */
        if (parser->current.type == TOKEN_ASSIGN) {
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *init;
            if (parser->current.type == TOKEN_LBRACE) {
                /* Stage 44: struct global initialized with brace list. */
                if (decl->decl_type != TYPE_STRUCT) {
                    fprintf(stderr,
                            "error: brace initializer not valid for non-struct global '%s'\n",
                            d.name);
                    exit(1);
                }
                init = parse_initializer(parser);
            } else {
                init = parse_primary(parser);
                if (init->type != AST_INT_LITERAL && init->type != AST_CHAR_LITERAL &&
                    init->type != AST_STRING_LITERAL) {
                    fprintf(stderr,
                            "error: non-constant initializer for global '%s'\n", d.name);
                    exit(1);
                }
                if (init->type == AST_STRING_LITERAL && decl->decl_type != TYPE_POINTER) {
                    fprintf(stderr,
                            "error: string literal can only initialize a pointer\n");
                    exit(1);
                }
            }
            ast_add_child(decl, init);
        }
        if (parser->current.type != TOKEN_COMMA) {
            parser_expect(parser, TOKEN_SEMICOLON);
            return decl;
        }
        /* Stage 22-02: comma-separated declarator list at file scope. */
        ASTNode *list = ast_new(AST_DECL_LIST, NULL);
        ast_add_child(list, decl);
        while (parser->current.type == TOKEN_COMMA) {
            parser->current = lexer_next_token(parser->lexer);
            ParsedDeclarator d2 = parse_declarator(parser);
            if (d2.is_array || d2.is_function) {
                fprintf(stderr,
                        "error: invalid declarator in file-scope list\n");
                exit(1);
            }
            if (parser_find_function(parser, d2.name)) {
                fprintf(stderr,
                        "error: '%s' redeclared as a different kind of symbol\n", d2.name);
                exit(1);
            }
            TypeKind k2 = (d2.pointer_count > 0 || base_kind == TYPE_POINTER)
                          ? TYPE_POINTER : base_kind;
            Type *ft2 = base_type;
            for (int i = 0; i < d2.pointer_count; i++)
                ft2 = type_pointer(ft2);
            Type *reg_ft2 = (k2 == TYPE_POINTER) ? ft2 : NULL;
            parser_register_global(parser, d2.name, k2, sc, reg_ft2);
            ASTNode *next_decl = ast_new(AST_DECLARATION, d2.name);
            next_decl->storage_class = sc;
            next_decl->is_const = (ds.is_const && d2.pointer_count == 0) ? 1 : 0;
            if (d2.pointer_count > 0 || base_kind == TYPE_POINTER) {
                next_decl->decl_type = TYPE_POINTER;
                next_decl->full_type = ft2;
            } else {
                next_decl->decl_type = base_kind;
                next_decl->is_unsigned = !base_type->is_signed;
            }
            if (parser->current.type == TOKEN_ASSIGN) {
                parser->current = lexer_next_token(parser->lexer);
                ASTNode *init2 = parse_primary(parser);
                if (init2->type != AST_INT_LITERAL && init2->type != AST_CHAR_LITERAL) {
                    fprintf(stderr,
                            "error: non-constant initializer for global '%s'\n", d2.name);
                    exit(1);
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
    ASTNode *func = ast_new(AST_FUNCTION_DECL, d.name);
    func->decl_type = return_kind;
    func->storage_class = sc;
    if (d.pointer_count > 0)
        func->full_type = full_type;

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
    }
    parser_expect(parser, TOKEN_RPAREN);

    if (parser->current.type == TOKEN_ASSIGN) {
        fprintf(stderr,
                "error: function declaration cannot have an initializer\n");
        exit(1);
    }

    int param_count = func->child_count;
    int is_definition = (parser->current.type == TOKEN_LBRACE);

    /* Stage 26: function definitions require named parameters. */
    if (is_definition) {
        for (int i = 0; i < param_count; i++) {
            if (func->children[i]->type == AST_PARAM &&
                func->children[i]->value[0] == '\0') {
                fprintf(stderr,
                        "error: unnamed parameter in definition of '%s'\n",
                        d.name);
                exit(1);
            }
        }
    }

    /* Collect parameter types for registration. */
    TypeKind param_types[FUNC_MAX_PARAMS];
    if (param_count > FUNC_MAX_PARAMS) {
        fprintf(stderr,
                "error: function '%s' has %d parameters; max supported is %d\n",
                d.name, param_count, FUNC_MAX_PARAMS);
        exit(1);
    }
    for (int i = 0; i < param_count; i++)
        param_types[i] = func->children[i]->decl_type;

    /* Register before parsing the body so self-calls resolve. */
    parser_register_function(parser, d.name, param_count, is_definition,
                             return_kind, param_types, sc);

    if (is_definition) {
        ast_add_child(func, parse_block(parser));
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
 */
ASTNode *parse_translation_unit(Parser *parser) {
    ASTNode *unit = ast_new(AST_TRANSLATION_UNIT, NULL);
    do {
        ASTNode *ext_decl = parse_external_declaration(parser);
        ast_add_child(unit, ext_decl);
    } while (parser->current.type != TOKEN_EOF);
    parser_expect(parser, TOKEN_EOF);
    return unit;
}
