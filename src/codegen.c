#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "type.h"
#include "util.h"

static int type_kind_bytes(TypeKind kind) {
    switch (kind) {
    case TYPE_VOID:     return 0; /* never directly allocated */
    case TYPE_BOOL:     return 1;
    case TYPE_CHAR:     return 1;
    case TYPE_SHORT:    return 2;
    case TYPE_INT:      return 4;
    case TYPE_LONG:               return 8;
    case TYPE_LONG_LONG:          return 8;
    case TYPE_UNSIGNED_LONG_LONG: return 8;
    case TYPE_POINTER:            return 8;
    case TYPE_ARRAY:              return 0; /* size lives on full_type; caller uses that */
    case TYPE_FUNCTION:           return 0; /* never directly allocated; base of FP pointer */
    case TYPE_STRUCT:             return 0; /* size lives on full_type; caller uses that */
    case TYPE_UNION:              return 0; /* size lives on full_type; caller uses that */
    }
    return 4;
}

/*
 * Emit instructions to convert the value currently in rax/eax from
 * `src` to `dst` following assignment-style rules: widen with
 * sign-extend, narrow by sign-extending the low byte/word back into
 * eax, or no-op when the two kinds are the same size. Narrowing to
 * int (4 bytes) is implicit because eax already holds the low 32
 * bits of rax. Used at call sites (argument → parameter) and at
 * return statements (expression → declared return type).
 */
static void emit_convert(CodeGen *cg, TypeKind src, TypeKind dst) {
    int src_sz = type_kind_bytes(src);
    int dst_sz = type_kind_bytes(dst);
    if (src_sz == dst_sz) return;
    if (dst_sz == 8) {
        fprintf(cg->output, "    movsxd rax, eax\n");
    } else if (dst_sz == 2) {
        fprintf(cg->output, "    movsx eax, ax\n");
    } else if (dst_sz == 1) {
        fprintf(cg->output, "    movsx eax, al\n");
    }
    /* dst_sz == 4 and src_sz == 8: low 32 bits of rax are already in
     * eax, so no explicit instruction is needed. */
}

/*
 * Stage 63: normalize the value in eax/rax to 0 or 1 for _Bool storage.
 * Any nonzero value becomes 1; zero stays 0. `is_long_value` selects
 * whether to test the full rax (1) or just eax (0).
 */
static void emit_bool_normalize(CodeGen *cg, int is_long_value) {
    if (is_long_value) {
        fprintf(cg->output, "    test rax, rax\n");
    } else {
        fprintf(cg->output, "    test eax, eax\n");
    }
    fprintf(cg->output, "    setne al\n");
    fprintf(cg->output, "    movzx eax, al\n");
}

/*
 * Stage 12-06: the integer literal `0` is the C null pointer constant.
 * The lexer drops any L/l suffix from the value text, so a value
 * string of "0" matches both `0` and `0L`.
 */
static int is_null_pointer_constant(ASTNode *node) {
    return node && node->type == AST_INT_LITERAL &&
           strcmp(node->value, "0") == 0;
}

/*
 * Stage 12-04: two pointer Types are compatible only when their full
 * chains agree on every level — same kind at each step, same integer
 * base. Mismatched bases (e.g. `int *` vs `char *`) are rejected.
 */
static int pointer_types_equal(Type *a, Type *b) {
    while (a && b) {
        if (a->kind != b->kind) return 0;
        if (a->kind != TYPE_POINTER) return 1;
        a = a->base;
        b = b->base;
    }
    return a == b;
}

/* Stage 38: true when `t` is exactly `void *` (one pointer level over void). */
static int is_void_ptr(Type *t) {
    return t && t->kind == TYPE_POINTER && t->base && t->base->kind == TYPE_VOID;
}

/*
 * Stage 38: pointer assignment compatibility. `void *` is assignable
 * to/from any object pointer type (non-function pointer). Delegates to
 * pointer_types_equal for all other cases.
 */
static int pointer_types_assignable(Type *dst, Type *src) {
    if (!dst || dst->kind != TYPE_POINTER) return 0;
    if (!src || src->kind != TYPE_POINTER) return 0;
    /* void* <-> function pointer is not allowed */
    if (dst->base && dst->base->kind == TYPE_FUNCTION) return pointer_types_equal(dst, src);
    if (src->base && src->base->kind == TYPE_FUNCTION) return pointer_types_equal(dst, src);
    if (is_void_ptr(dst) || is_void_ptr(src)) return 1;
    return pointer_types_equal(dst, src);
}

/*
 * Stage 25-02: deep equality check for two function-pointer types.
 * Both a and b must be TYPE_POINTER → TYPE_FUNCTION chains.
 * Compares return type kind, parameter count, and each parameter kind.
 */
static int func_ptr_types_equal_cg(Type *a, Type *b) {
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

/* Stage 25-02: map a TypeKind to its singleton Type*. */
static Type *type_from_kind(TypeKind kind) {
    switch (kind) {
    case TYPE_VOID:               return type_void();
    case TYPE_CHAR:               return type_char();
    case TYPE_SHORT:              return type_short();
    case TYPE_LONG:               return type_long();
    case TYPE_LONG_LONG:          return type_long_long();
    case TYPE_UNSIGNED_LONG_LONG: return type_unsigned_long_long();
    case TYPE_INT:
    default:                      return type_int();
    }
}

/*
 * Stage 25-02: build a TYPE_POINTER → TYPE_FUNCTION type chain from an
 * AST_FUNCTION_DECL node. The result represents the function's type as a
 * function pointer so it can be compared against the LHS of an assignment.
 */
static Type *build_func_designator_type(ASTNode *func_decl) {
    Type *return_type = (func_decl->decl_type == TYPE_POINTER && func_decl->full_type)
                        ? func_decl->full_type
                        : type_from_kind(func_decl->decl_type);
    Type *param_types[FUNC_TYPE_MAX_PARAMS];
    int param_count = 0;
    for (int i = 0; i < func_decl->child_count && param_count < FUNC_TYPE_MAX_PARAMS; i++) {
        ASTNode *child = func_decl->children[i];
        if (child->type == AST_PARAM) {
            param_types[param_count++] =
                (child->decl_type == TYPE_POINTER && child->full_type)
                ? child->full_type
                : type_from_kind(child->decl_type);
        }
    }
    return type_pointer(type_function(return_type, param_count, param_types));
}

/*
 * Look up a function's AST_FUNCTION_DECL node by name in the current
 * translation unit so call sites can see the callee's declared
 * parameter types for argument conversion. If multiple declarations
 * exist (forward declaration plus definition), the first is
 * returned — the parser enforces that their signatures match.
 */
static ASTNode *codegen_find_function_decl(CodeGen *cg, const char *name) {
    if (!cg->tu_root) return NULL;
    for (int i = 0; i < cg->tu_root->child_count; i++) {
        ASTNode *c = cg->tu_root->children[i];
        if (c->type == AST_FUNCTION_DECL && strcmp(c->value, name) == 0) {
            return c;
        }
    }
    return NULL;
}

/*
 * Emit a size-appropriate load of a local into rax/eax.
 * For signed small types, sign-extend; for unsigned small types, zero-extend.
 * int loads into eax (zero-extends rax implicitly in x86-64); long loads rax.
 */
static void emit_load_local(CodeGen *cg, int offset, int size, int is_unsigned) {
    switch (size) {
    case 1:
        if (is_unsigned)
            fprintf(cg->output, "    movzx eax, byte [rbp - %d]\n", offset);
        else
            fprintf(cg->output, "    movsx eax, byte [rbp - %d]\n", offset);
        break;
    case 2:
        if (is_unsigned)
            fprintf(cg->output, "    movzx eax, word [rbp - %d]\n", offset);
        else
            fprintf(cg->output, "    movsx eax, word [rbp - %d]\n", offset);
        break;
    case 8: fprintf(cg->output, "    mov rax, [rbp - %d]\n", offset); break;
    case 4:
    default:
        fprintf(cg->output, "    mov eax, [rbp - %d]\n", offset);
        break;
    }
}

/*
 * Store the current value into a local, truncating to the local's
 * storage size. `src_is_long` tells the helper whether the value is
 * already in the full rax (src_is_long=1) or only in eax as a 32-bit
 * value (src_is_long=0). For an 8-byte destination with a 32-bit signed
 * source the value is sign-extended via movsxd. For an unsigned 32-bit
 * source pass src_is_long=1 — x86-64 32-bit writes already zero-extend.
 */
static void emit_store_local(CodeGen *cg, int offset, int size, int src_is_long) {
    switch (size) {
    case 1: fprintf(cg->output, "    mov [rbp - %d], al\n", offset); break;
    case 2: fprintf(cg->output, "    mov [rbp - %d], ax\n", offset); break;
    case 8:
        if (!src_is_long) {
            fprintf(cg->output, "    movsxd rax, eax\n");
        }
        fprintf(cg->output, "    mov [rbp - %d], rax\n", offset);
        break;
    case 4:
    default:
        fprintf(cg->output, "    mov [rbp - %d], eax\n", offset);
        break;
    }
}

void codegen_init(CodeGen *cg, FILE *output) {
    cg->output = output;
    cg->label_count = 0;
    cg->local_count = 0;
    cg->global_count = 0;
    cg->stack_offset = 0;
    cg->scope_start = 0;
    cg->push_depth = 0;
    cg->has_frame = 0;
    cg->break_depth = 0;
    cg->switch_depth = 0;
    cg->user_label_count = 0;
    cg->current_func = NULL;
    cg->current_return_type = TYPE_INT;
    cg->current_return_full_type = NULL;
    cg->tu_root = NULL;
    cg->string_pool_count = 0;
    cg->warnings_are_errors = 0;
    cg->local_static_count = 0;
    cg->variadic_reg_save_offset = 0;
    cg->variadic_named_gp_params = 0;
    cg->variadic_named_stack_params = 0;
}

/* Stage 66/70-03: warn with a variable name embedded.
 * Position is omitted (codegen has no token info); g_warnings_are_errors
 * is checked inside compile_warning_at. */
static void codegen_warn_const_discard(CodeGen *cg, const char *prefix,
                                        const char *varname) {
    (void)cg; /* warnings_are_errors now routed through g_warnings_are_errors */
    compile_warning_at(NULL, 0, 0,
        "warning: %s '%s' discards 'const' qualifier from pointer target type\n",
        prefix, varname);
}

/*
 * Walk a function body collecting every AST_LABEL_STATEMENT node's
 * name into the per-function user-label table. Duplicate labels
 * within the same function are rejected here; `goto` resolution
 * against the completed table happens during emission.
 */
static void collect_user_labels(CodeGen *cg, ASTNode *node) {
    if (!node) return;
    if (node->type == AST_LABEL_STATEMENT) {
        for (int i = 0; i < cg->user_label_count; i++) {
            if (strcmp(cg->user_labels[i], node->value) == 0) {
                compile_error( "error: duplicate label '%s' in function '%s'\n",
                        node->value, cg->current_func);
            }
        }
        if (cg->user_label_count >= MAX_USER_LABELS) {
            compile_error( "error: too many labels in function '%s' (max %d)\n",
                    cg->current_func, MAX_USER_LABELS);
        }
        strncpy(cg->user_labels[cg->user_label_count], node->value, 255);
        cg->user_labels[cg->user_label_count][255] = '\0';
        cg->user_label_count++;
    }
    for (int i = 0; i < node->child_count; i++) {
        collect_user_labels(cg, node->children[i]);
    }
}

static int user_label_defined(CodeGen *cg, const char *name) {
    for (int i = 0; i < cg->user_label_count; i++) {
        if (strcmp(cg->user_labels[i], name) == 0) return 1;
    }
    return 0;
}

/*
 * Walk the subtree of a switch body, collecting every AST_CASE_SECTION
 * and AST_DEFAULT_SECTION node pointer into `ctx` and assigning each a
 * fresh label. Descent stops at nested AST_SWITCH_STATEMENT nodes
 * because labels inside a nested switch belong to that inner switch.
 * A case node's case-value child (children[0]) is not descended into.
 */
static void collect_switch_labels(CodeGen *cg, ASTNode *node, SwitchCtx *ctx) {
    if (!node) return;
    if (node->type == AST_CASE_SECTION) {
        if (ctx->count >= MAX_SWITCH_LABELS) {
            compile_error( "error: too many case/default labels in switch (max %d)\n",
                    MAX_SWITCH_LABELS);
        }
        ctx->nodes[ctx->count] = node;
        ctx->labels[ctx->count] = cg->label_count++;
        ctx->count++;
        if (node->child_count > 1) {
            collect_switch_labels(cg, node->children[1], ctx);
        }
        return;
    }
    if (node->type == AST_DEFAULT_SECTION) {
        if (ctx->count >= MAX_SWITCH_LABELS) {
            compile_error( "error: too many case/default labels in switch (max %d)\n",
                    MAX_SWITCH_LABELS);
        }
        int lbl = cg->label_count++;
        ctx->nodes[ctx->count] = node;
        ctx->labels[ctx->count] = lbl;
        ctx->count++;
        ctx->default_label = lbl;
        if (node->child_count > 0) {
            collect_switch_labels(cg, node->children[0], ctx);
        }
        return;
    }
    if (node->type == AST_SWITCH_STATEMENT) {
        return; /* nested switch — its labels belong to it */
    }
    for (int i = 0; i < node->child_count; i++) {
        collect_switch_labels(cg, node->children[i], ctx);
    }
}

static int switch_lookup_label(SwitchCtx *ctx, ASTNode *node) {
    for (int i = 0; i < ctx->count; i++) {
        if (ctx->nodes[i] == node) return ctx->labels[i];
    }
    return -1;
}

static LocalVar *codegen_find_var(CodeGen *cg, const char *name) {
    /* Walk backward so the innermost (most recently declared) shadow wins. */
    for (int i = cg->local_count - 1; i >= 0; i--) {
        if (strcmp(cg->locals[i].name, name) == 0)
            return &cg->locals[i];
    }
    return NULL;
}

/*
 * Allocate a local of `size` bytes with `align`-byte natural
 * alignment. Stack grows down from rbp, so the variable's
 * rbp-relative offset is advanced by `size` and then aligned up to a
 * multiple of `align`.
 *
 * Stage 12-02: also records the declared kind and (for pointers) the
 * full Type chain so address-of and dereference codegen can recover
 * the pointed-to type.
 *
 * Stage 13-01: split alignment from size so array locals (whose total
 * size is element_size * length and is not a power of two in general)
 * align to the element's natural alignment instead of their total
 * byte count.
 */
static int codegen_add_var(CodeGen *cg, const char *name, int size, int align,
                           TypeKind kind, Type *full_type) {
    cg->stack_offset += size;
    cg->stack_offset = (cg->stack_offset + align - 1) & ~(align - 1);
    strncpy(cg->locals[cg->local_count].name, name, 255);
    cg->locals[cg->local_count].name[255] = '\0';
    cg->locals[cg->local_count].offset = cg->stack_offset;
    cg->locals[cg->local_count].size = size;
    cg->locals[cg->local_count].kind = kind;
    cg->locals[cg->local_count].full_type = full_type;
    cg->locals[cg->local_count].is_const = 0;
    cg->local_count++;
    return cg->stack_offset;
}

/*
 * Stage 12-02: recover a `Type *` for a local. For pointer locals
 * `full_type` is the pointer chain head; for integer locals we
 * synthesize the corresponding singleton.
 */
static Type *local_var_type(LocalVar *lv) {
    if (lv->full_type) return lv->full_type;
    Type *t;
    switch (lv->kind) {
    case TYPE_CHAR:               t = type_char(); break;
    case TYPE_SHORT:              t = type_short(); break;
    case TYPE_LONG:               t = type_long(); break;
    case TYPE_LONG_LONG:          t = type_long_long(); break;
    case TYPE_UNSIGNED_LONG_LONG: t = type_unsigned_long_long(); break;
    case TYPE_STRUCT:             return lv->full_type; /* always set for struct/union locals */
    case TYPE_UNION:              return lv->full_type;
    case TYPE_INT:
    default:                      t = type_int(); break;
    }
    /* Stage 66: const int x → &x yields const int*; return a const copy. */
    if (lv->is_const) t = type_const_copy(t);
    return t;
}

/* Stage 22-01: global variable helpers. */
static GlobalVar *codegen_find_global(CodeGen *cg, const char *name) {
    for (int i = 0; i < cg->global_count; i++) {
        if (strcmp(cg->globals[i].name, name) == 0)
            return &cg->globals[i];
    }
    return NULL;
}

static Type *global_var_type(GlobalVar *gv) {
    if (gv->full_type) return gv->full_type;
    Type *t;
    switch (gv->kind) {
    case TYPE_CHAR:               t = type_char(); break;
    case TYPE_SHORT:              t = type_short(); break;
    case TYPE_LONG:               t = type_long(); break;
    case TYPE_LONG_LONG:          t = type_long_long(); break;
    case TYPE_UNSIGNED_LONG_LONG: t = type_unsigned_long_long(); break;
    case TYPE_INT:
    default:                      t = type_int(); break;
    }
    /* Stage 66: const global x → &x yields const T*; return a const copy. */
    if (gv->is_const) t = type_const_copy(t);
    return t;
}

static void emit_load_global(CodeGen *cg, const char *name, int size, int is_unsigned) {
    switch (size) {
    case 1:
        if (is_unsigned)
            fprintf(cg->output, "    movzx eax, byte [rel %s]\n", name);
        else
            fprintf(cg->output, "    movsx eax, byte [rel %s]\n", name);
        break;
    case 2:
        if (is_unsigned)
            fprintf(cg->output, "    movzx eax, word [rel %s]\n", name);
        else
            fprintf(cg->output, "    movsx eax, word [rel %s]\n", name);
        break;
    case 8: fprintf(cg->output, "    mov rax, [rel %s]\n", name); break;
    case 4:
    default:
        fprintf(cg->output, "    mov eax, [rel %s]\n", name);
        break;
    }
}

static void emit_store_global(CodeGen *cg, const char *name, int size,
                              int src_is_long) {
    switch (size) {
    case 1: fprintf(cg->output, "    mov [rel %s], al\n", name); break;
    case 2: fprintf(cg->output, "    mov [rel %s], ax\n", name); break;
    case 8:
        /* src_is_long=1 for both 64-bit values and unsigned 32-bit values
         * (x86-64 zero-extends 32-bit writes automatically). */
        if (!src_is_long)
            fprintf(cg->output, "    movsxd rax, eax\n");
        fprintf(cg->output, "    mov [rel %s], rax\n", name);
        break;
    case 4:
    default:
        fprintf(cg->output, "    mov [rel %s], eax\n", name);
        break;
    }
}

static void codegen_expression(CodeGen *cg, ASTNode *node);

/* Stage 78: forward declarations needed because emit_array_index_addr calls
 * emit_member_addr / emit_arrow_addr which are defined later in the file. */
static StructField *find_struct_field(Type *st, const char *name);
static StructField *emit_member_addr(CodeGen *cg, ASTNode *node);
static StructField *emit_arrow_addr(CodeGen *cg, ASTNode *node);

/*
 * Emit code to compute the address of an array/pointer subscript
 * `b[i]` into rax. Returns the element Type so the caller can pick
 * the matching load/store width. The base must be an identifier
 * referring to either an array local (Stage 13-02) or a pointer local
 * (Stage 13-03 — needed once a pointer parameter can receive a
 * decayed array). For an array the slot's address is the base; for a
 * pointer the slot's value is the base. The index expression must be
 * integer-typed; it is sign-extended to 64 bits and multiplied by
 * `sizeof(element)` before being added to the base address. The helper
 * leaves rbx clobbered.
 */
static Type *emit_array_index_addr(CodeGen *cg, ASTNode *node) {
    ASTNode *base_node = node->children[0];
    ASTNode *index_node = node->children[1];
    /* Stage 28-04: (*ptr_to_array)[idx] — base is a deref of a pointer to array.
     * Load the pointer value (the array's base address) and use the array's
     * element type for index scaling. */
    if (base_node->type == AST_DEREF) {
        ASTNode *inner = base_node->children[0];
        if (inner->type != AST_VAR_REF) {
            compile_error( "error: subscript base must be an identifier\n");
        }
        LocalVar *plv = codegen_find_var(cg, inner->value);
        if (!plv || plv->kind != TYPE_POINTER || !plv->full_type ||
            !plv->full_type->base || plv->full_type->base->kind != TYPE_ARRAY) {
            compile_error( "error: subscript base must be a pointer to array\n");
        }
        if (plv->is_static)
            fprintf(cg->output, "    mov rax, [rel %s]\n", plv->static_label);
        else
            fprintf(cg->output, "    mov rax, [rbp - %d]\n", plv->offset);
        Type *element = plv->full_type->base->base;
        int elem_size = type_size(element);
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;
        codegen_expression(cg, index_node);
        TypeKind index_kind = index_node->result_type;
        if (index_kind != TYPE_INT && index_kind != TYPE_LONG) {
            compile_error( "error: array subscript index must be an integer\n");
        }
        if (index_kind != TYPE_LONG) {
            fprintf(cg->output, "    movsxd rax, eax\n");
        }
        if (elem_size != 1) {
            fprintf(cg->output, "    imul rax, rax, %d\n", elem_size);
        }
        fprintf(cg->output, "    pop rbx\n");
        cg->push_depth--;
        fprintf(cg->output, "    add rax, rbx\n");
        return element;
    }
    /* Stage 42: nested subscript — base is itself an array-index expression
     * (e.g. names[0][1] where names is char *[]).  Evaluate the inner
     * subscript address, load the pointer stored there, then use it as the
     * pointer base for the outer index. */
    if (base_node->type == AST_ARRAY_INDEX) {
        Type *inner_element = emit_array_index_addr(cg, base_node);
        if (inner_element->kind != TYPE_POINTER) {
            compile_error( "error: subscript base is not a pointer or array\n");
        }
        Type *element = inner_element->base;
        if (element->kind == TYPE_VOID) {
            compile_error( "error: cannot subscript void pointer\n");
        }
        fprintf(cg->output, "    mov rax, [rax]\n");
        int elem_size = type_size(element);
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;
        codegen_expression(cg, index_node);
        TypeKind index_kind = index_node->result_type;
        if (index_kind != TYPE_INT && index_kind != TYPE_LONG) {
            compile_error( "error: array subscript index must be an integer\n");
        }
        if (index_kind != TYPE_LONG) {
            fprintf(cg->output, "    movsxd rax, eax\n");
        }
        if (elem_size != 1) {
            fprintf(cg->output, "    imul rax, rax, %d\n", elem_size);
        }
        fprintf(cg->output, "    pop rbx\n");
        cg->push_depth--;
        fprintf(cg->output, "    add rax, rbx\n");
        return element;
    }
    /* Stage 78: member access (.) as subscript base — e.g. n.values[i].
     * emit_member_addr leaves the field's address in rax.  For an array
     * field that address is the base of the array; for a pointer field we
     * must load the pointer value from that address first. */
    if (base_node->type == AST_MEMBER_ACCESS) {
        StructField *f = emit_member_addr(cg, base_node);
        Type *element;
        if (f->kind == TYPE_ARRAY && f->full_type && f->full_type->base) {
            element = f->full_type->base;
        } else if (f->kind == TYPE_POINTER && f->full_type && f->full_type->base) {
            element = f->full_type->base;
            fprintf(cg->output, "    mov rax, [rax]\n");
        } else {
            compile_error("error: subscript applied to member '%s' which is not an array or pointer\n",
                    base_node->value);
        }
        int elem_size = type_size(element);
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;
        codegen_expression(cg, index_node);
        TypeKind index_kind = index_node->result_type;
        if (index_kind != TYPE_INT && index_kind != TYPE_LONG) {
            compile_error("error: array subscript index must be an integer\n");
        }
        if (index_kind != TYPE_LONG) {
            fprintf(cg->output, "    movsxd rax, eax\n");
        }
        if (elem_size != 1) {
            fprintf(cg->output, "    imul rax, rax, %d\n", elem_size);
        }
        fprintf(cg->output, "    pop rbx\n");
        cg->push_depth--;
        fprintf(cg->output, "    add rax, rbx\n");
        return element;
    }
    /* Stage 78: arrow access (->) as subscript base — e.g. p->values[i].
     * emit_arrow_addr leaves the field's address in rax.  Same array/pointer
     * dispatch as the member-access case above. */
    if (base_node->type == AST_ARROW_ACCESS) {
        StructField *f = emit_arrow_addr(cg, base_node);
        Type *element;
        if (f->kind == TYPE_ARRAY && f->full_type && f->full_type->base) {
            element = f->full_type->base;
        } else if (f->kind == TYPE_POINTER && f->full_type && f->full_type->base) {
            element = f->full_type->base;
            fprintf(cg->output, "    mov rax, [rax]\n");
        } else {
            compile_error("error: subscript applied to member '%s' which is not an array or pointer\n",
                    base_node->value);
        }
        int elem_size = type_size(element);
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;
        codegen_expression(cg, index_node);
        TypeKind index_kind = index_node->result_type;
        if (index_kind != TYPE_INT && index_kind != TYPE_LONG) {
            compile_error("error: array subscript index must be an integer\n");
        }
        if (index_kind != TYPE_LONG) {
            fprintf(cg->output, "    movsxd rax, eax\n");
        }
        if (elem_size != 1) {
            fprintf(cg->output, "    imul rax, rax, %d\n", elem_size);
        }
        fprintf(cg->output, "    pop rbx\n");
        cg->push_depth--;
        fprintf(cg->output, "    add rax, rbx\n");
        return element;
    }
    if (base_node->type != AST_VAR_REF) {
        compile_error( "error: subscript base must be an identifier\n");
    }
    LocalVar *lv = codegen_find_var(cg, base_node->value);
    GlobalVar *gv = lv ? NULL : codegen_find_global(cg, base_node->value);
    if (!lv && !gv) {
        compile_error( "error: undeclared variable '%s'\n", base_node->value);
    }
    Type *element;
    if (lv) {
        if (lv->kind == TYPE_ARRAY) {
            element = lv->full_type->base;
            /* Array statics are rejected at declaration time; is_static is
             * always false here, so no static-label branch needed. */
            fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset);
        } else if (lv->kind == TYPE_POINTER) {
            element = lv->full_type->base;
            /* Stage 38: subscript on void * is not allowed. */
            if (element && element->kind == TYPE_VOID) {
                compile_error(
                        "error: cannot subscript void pointer '%s'\n",
                        base_node->value);
            }
            if (lv->is_static)
                fprintf(cg->output, "    mov rax, [rel %s]\n", lv->static_label);
            else
                fprintf(cg->output, "    mov rax, [rbp - %d]\n", lv->offset);
        } else {
            compile_error(
                    "error: subscript base '%s' is not an array or pointer\n",
                    base_node->value);
        }
    } else {
        if (gv->kind == TYPE_ARRAY) {
            element = gv->full_type->base;
            fprintf(cg->output, "    lea rax, [rel %s]\n", gv->name);
        } else if (gv->kind == TYPE_POINTER) {
            element = gv->full_type->base;
            /* Stage 38: subscript on void * is not allowed. */
            if (element && element->kind == TYPE_VOID) {
                compile_error(
                        "error: cannot subscript void pointer '%s'\n",
                        base_node->value);
            }
            fprintf(cg->output, "    mov rax, [rel %s]\n", gv->name);
        } else {
            compile_error(
                    "error: subscript base '%s' is not an array or pointer\n",
                    base_node->value);
        }
    }
    int elem_size = type_size(element);

    fprintf(cg->output, "    push rax\n");
    cg->push_depth++;
    codegen_expression(cg, index_node);
    TypeKind index_kind = index_node->result_type;
    if (index_kind != TYPE_INT && index_kind != TYPE_LONG) {
        compile_error( "error: array subscript index must be an integer\n");
    }
    if (index_kind != TYPE_LONG) {
        fprintf(cg->output, "    movsxd rax, eax\n");
    }
    if (elem_size != 1) {
        fprintf(cg->output, "    imul rax, rax, %d\n", elem_size);
    }
    fprintf(cg->output, "    pop rbx\n");
    cg->push_depth--;
    fprintf(cg->output, "    add rax, rbx\n");
    return element;
}

/*
 * Stage 31: look up a named field in a TYPE_STRUCT Type.
 * Returns a pointer into the Type's field table, or NULL if not found.
 */
static StructField *find_struct_field(Type *st, const char *name) {
    for (int i = 0; i < st->field_count; i++) {
        if (strcmp(st->fields[i].name, name) == 0)
            return &st->fields[i];
    }
    return NULL;
}

/*
 * Stage 31: emit the address of a struct field accessed via "." and
 * leave it in rax.  The base must be an AST_VAR_REF naming a local or
 * global struct variable.  Returns the StructField descriptor so the
 * caller knows the field's size and type.
 *
 * For a local struct at rbp-relative offset O, field at byte offset F:
 *   field address = rbp - O + F  =  rbp - (O - F)
 */
static StructField *emit_member_addr(CodeGen *cg, ASTNode *node) {
    ASTNode *base = node->children[0];
    const char *field_name = node->value;

    /* Stage 34: (*ptr).field — load pointer, add field offset. */
    if (base->type == AST_DEREF) {
        ASTNode *ptr_expr = base->children[0];
        if (ptr_expr->type != AST_VAR_REF) {
            compile_error( "error: '.' via dereference: pointer must be an identifier\n");
        }
        LocalVar *plv = codegen_find_var(cg, ptr_expr->value);
        if (!plv) {
            compile_error( "error: undeclared variable '%s'\n", ptr_expr->value);
        }
        if (plv->kind != TYPE_POINTER || !plv->full_type ||
            !plv->full_type->base || (plv->full_type->base->kind != TYPE_STRUCT &&
                                      plv->full_type->base->kind != TYPE_UNION)) {
            compile_error(
                    "error: '.' via dereference: '%s' is not a pointer to struct/union\n",
                    ptr_expr->value);
        }
        Type *st = plv->full_type->base;
        StructField *f = find_struct_field(st, field_name);
        if (!f) {
            compile_error( "error: '%s' has no member '%s'\n",
                    type_kind_name(st->kind), field_name);
        }
        if (plv->is_static)
            fprintf(cg->output, "    mov rax, [rel %s]\n", plv->static_label);
        else
            fprintf(cg->output, "    mov rax, [rbp - %d]\n", plv->offset);
        if (f->offset != 0)
            fprintf(cg->output, "    add rax, %d\n", f->offset);
        return f;
    }

    /* Stage 35: chained member access — base is itself a member access (e.g. r.origin.x). */
    if (base->type == AST_MEMBER_ACCESS) {
        StructField *inner_f = emit_member_addr(cg, base);
        if ((inner_f->kind != TYPE_STRUCT && inner_f->kind != TYPE_UNION) ||
            !inner_f->full_type) {
            compile_error( "error: '.' applied to non-struct/union member '%s'\n",
                    base->value);
        }
        StructField *f = find_struct_field(inner_f->full_type, field_name);
        if (!f) {
            compile_error( "error: '%s' has no member '%s'\n",
                    type_kind_name(inner_f->kind), field_name);
        }
        if (f->offset != 0)
            fprintf(cg->output, "    add rax, %d\n", f->offset);
        return f;
    }

    /* Stage 35: array-element member access — base is array subscript (e.g. points[0].x). */
    if (base->type == AST_ARRAY_INDEX) {
        Type *element = emit_array_index_addr(cg, base);
        if (element->kind != TYPE_STRUCT && element->kind != TYPE_UNION) {
            compile_error( "error: '.' applied to non-struct/union array element\n");
        }
        StructField *f = find_struct_field(element, field_name);
        if (!f) {
            compile_error( "error: '%s' has no member '%s'\n",
                    type_kind_name(element->kind), field_name);
        }
        if (f->offset != 0)
            fprintf(cg->output, "    add rax, %d\n", f->offset);
        return f;
    }

    if (base->type != AST_VAR_REF) {
        compile_error( "error: '.' base must be an identifier\n");
    }
    LocalVar *lv = codegen_find_var(cg, base->value);
    if (lv) {
        if ((lv->kind != TYPE_STRUCT && lv->kind != TYPE_UNION) || !lv->full_type) {
            compile_error( "error: '.' applied to non-struct/union '%s'\n", base->value);
        }
        StructField *f = find_struct_field(lv->full_type, field_name);
        if (!f) {
            compile_error( "error: '%s' has no member '%s'\n",
                    type_kind_name(lv->kind), field_name);
        }
        fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset - f->offset);
        return f;
    }
    /* Stage 44: fall back to global struct/union variable. */
    GlobalVar *gv = codegen_find_global(cg, base->value);
    if (!gv) {
        compile_error( "error: undeclared variable '%s'\n", base->value);
    }
    if ((gv->kind != TYPE_STRUCT && gv->kind != TYPE_UNION) || !gv->full_type) {
        compile_error( "error: '.' applied to non-struct/union '%s'\n", base->value);
    }
    StructField *f = find_struct_field(gv->full_type, field_name);
    if (!f) {
        compile_error( "error: '%s' has no member '%s'\n",
                type_kind_name(gv->kind), field_name);
    }
    if (f->offset != 0)
        fprintf(cg->output, "    lea rax, [rel %s + %d]\n", gv->name, f->offset);
    else
        fprintf(cg->output, "    lea rax, [rel %s]\n", gv->name);
    return f;
}

/*
 * Stage 31: emit the address of a struct field accessed via "->" and
 * leave it in rax.  The base may be an AST_VAR_REF naming a local
 * pointer-to-struct variable, or an AST_MEMBER_ACCESS (e.g. n.next->value).
 * Returns the StructField descriptor.
 */
static StructField *emit_arrow_addr(CodeGen *cg, ASTNode *node) {
    ASTNode *base = node->children[0];
    const char *field_name = node->value;

    /* Stage 37-02: chained dot-then-arrow (e.g. n.next->value). */
    if (base->type == AST_MEMBER_ACCESS) {
        StructField *inner = emit_member_addr(cg, base);
        if (inner->kind != TYPE_POINTER || !inner->full_type ||
            !inner->full_type->base || (inner->full_type->base->kind != TYPE_STRUCT &&
                                        inner->full_type->base->kind != TYPE_UNION)) {
            compile_error( "error: '->' applied to non-pointer-to-struct/union\n");
        }
        Type *st = inner->full_type->base;
        StructField *f = find_struct_field(st, field_name);
        if (!f) {
            compile_error( "error: '%s' has no member '%s'\n",
                    type_kind_name(st->kind), field_name);
        }
        fprintf(cg->output, "    mov rax, [rax]\n");
        if (f->offset != 0)
            fprintf(cg->output, "    add rax, %d\n", f->offset);
        return f;
    }

    /* Stage 41: support pointer-expression bases like (p+1)->field. */
    if (base->type != AST_VAR_REF) {
        codegen_expression(cg, base);
        Type *ptr_type = base->full_type;
        if (!ptr_type || ptr_type->kind != TYPE_POINTER ||
            !ptr_type->base || (ptr_type->base->kind != TYPE_STRUCT &&
                                ptr_type->base->kind != TYPE_UNION)) {
            compile_error( "error: '->' applied to non-pointer-to-struct/union\n");
        }
        StructField *f = find_struct_field(ptr_type->base, field_name);
        if (!f) {
            compile_error( "error: '%s' has no member '%s'\n",
                    type_kind_name(ptr_type->base->kind), field_name);
        }
        if (f->offset != 0)
            fprintf(cg->output, "    add rax, %d\n", f->offset);
        return f;
    }
    LocalVar *lv = codegen_find_var(cg, base->value);
    if (!lv) {
        compile_error( "error: undeclared variable '%s'\n", base->value);
    }
    if (lv->kind != TYPE_POINTER || !lv->full_type ||
        !lv->full_type->base || (lv->full_type->base->kind != TYPE_STRUCT &&
                                  lv->full_type->base->kind != TYPE_UNION)) {
        compile_error(
                "error: '->' applied to non-pointer-to-struct/union '%s'\n",
                base->value);
    }
    Type *st = lv->full_type->base;
    StructField *f = find_struct_field(st, field_name);
    if (!f) {
        compile_error( "error: '%s' has no member '%s'\n",
                type_kind_name(st->kind), field_name);
    }
    if (lv->is_static)
        fprintf(cg->output, "    mov rax, [rel %s]\n", lv->static_label);
    else
        fprintf(cg->output, "    mov rax, [rbp - %d]\n", lv->offset);
    if (f->offset != 0)
        fprintf(cg->output, "    add rax, %d\n", f->offset);
    return f;
}

/*
 * Conservative upper bound on stack bytes needed for locals: 8 bytes
 * per scalar/pointer declaration, and the array's actual byte count
 * plus 7 bytes of alignment slack per array declaration. The
 * prologue rounds the total up to 16.
 */
static int compute_decl_bytes(ASTNode *node) {
    if (!node) return 0;
    int total = 0;
    if (node->type == AST_DECLARATION) {
        if (node->decl_type == TYPE_ARRAY && node->full_type) {
            total = node->full_type->size + 7;
        } else if ((node->decl_type == TYPE_STRUCT || node->decl_type == TYPE_UNION) &&
                   node->full_type) {
            /* Struct/union: actual size plus alignment slack for stack alignment. */
            total = node->full_type->size + (node->full_type->alignment - 1);
        } else {
            total = 8;
        }
    }
    for (int i = 0; i < node->child_count; i++) {
        total += compute_decl_bytes(node->children[i]);
    }
    return total;
}

/*
 * Integer promotion: char/short are promoted to int for use in
 * arithmetic. int and long stay as-is. Stage 11-03: signed only.
 */
static TypeKind promote_kind(TypeKind t) {
    return (t == TYPE_LONG || t == TYPE_LONG_LONG || t == TYPE_UNSIGNED_LONG_LONG)
           ? TYPE_LONG : TYPE_INT;
}

/*
 * Common integer type for a binary arithmetic operator (after
 * promotion). Any long operand makes the result long; otherwise int.
 */
static TypeKind common_arith_kind(TypeKind a, TypeKind b) {
    if (promote_kind(a) == TYPE_LONG || promote_kind(b) == TYPE_LONG) {
        return TYPE_LONG;
    }
    return TYPE_INT;
}

/*
 * Stage 40: Usual arithmetic conversion signedness rule.
 * Returns 1 if the result of operating on two values with the given
 * promoted kinds and signedness should be treated as unsigned.
 * Implements the C99 UAC rules for the integer ranks we support
 * (int rank 3, long rank 4):
 *   same signedness    → use that signedness (higher rank wins kind)
 *   uint + slong       → signed (long can hold all uint values)
 *   ulong + sint       → unsigned (unsigned rank >= signed rank)
 *   uint + sint        → unsigned (same rank; unsigned wins)
 *   ulong + slong      → unsigned (same rank; unsigned wins)
 */
static int uac_is_unsigned(TypeKind ak, int au, TypeKind bk, int bu) {
    if (ak == TYPE_CHAR || ak == TYPE_SHORT) ak = TYPE_INT;
    if (bk == TYPE_CHAR || bk == TYPE_SHORT) bk = TYPE_INT;
    /* Normalize long long to long for rank comparisons — same 8-byte rank. */
    if (ak == TYPE_LONG_LONG || ak == TYPE_UNSIGNED_LONG_LONG) ak = TYPE_LONG;
    if (bk == TYPE_LONG_LONG || bk == TYPE_UNSIGNED_LONG_LONG) bk = TYPE_LONG;
    if (au == bu) return au;
    /* Different signedness. */
    int ulong_present = (au && ak == TYPE_LONG) || (bu && bk == TYPE_LONG);
    int slong_present = (!au && ak == TYPE_LONG) || (!bu && bk == TYPE_LONG);
    if (ulong_present) return 1;   /* unsigned long beats everything */
    if (slong_present) return 0;   /* signed long beats unsigned int */
    return 1;                       /* unsigned int beats signed int */
}

/*
 * Map a local's storage size to its post-promotion type. A size-8
 * local is long; any smaller size promotes to int when used in an
 * expression (sign-extended on load).
 */
static TypeKind type_kind_from_size(int size) {
    return (size == 8) ? TYPE_LONG : TYPE_INT;
}

/*
 * Resolve the C type of an expression for sizeof purposes — without
 * emitting any code and without applying the integer promotions that
 * `expr_result_type` applies to variable references.  The distinction
 * matters: sizeof(char_var) == 1, but sizeof(char_var + 1) == 4
 * because the addition promotes char to int.
 *
 * Rules:
 *   VAR_REF         → declared kind (TYPE_CHAR, TYPE_SHORT, TYPE_INT,
 *                      TYPE_LONG, TYPE_POINTER)
 *   DEREF(*p)       → kind of the pointee type
 *   ARRAY_INDEX     → kind of the element type
 *   binary arith    → apply promote_kind to each operand, then
 *                     common_arith_kind (matches the usual arithmetic
 *                     conversion rule used in expression evaluation)
 *   INT_LITERAL     → node->decl_type (TYPE_INT or TYPE_LONG)
 *   CHAR_LITERAL    → TYPE_INT (C char literals have type int)
 *   prefix/postfix  → kind of the operand variable
 *   assignment      → kind of the LHS
 *   cast            → node->decl_type
 *   function call   → declared return type (node->decl_type)
 */
static TypeKind sizeof_type_of_expr(CodeGen *cg, ASTNode *node) {
    if (!node) return TYPE_INT;
    switch (node->type) {
    case AST_INT_LITERAL:
        return node->decl_type;
    case AST_CHAR_LITERAL:
        return TYPE_INT;
    case AST_VAR_REF: {
        LocalVar *lv = codegen_find_var(cg, node->value);
        if (lv) return lv->kind;
        GlobalVar *gv = codegen_find_global(cg, node->value);
        if (gv) return gv->kind;
        return TYPE_INT;
    }
    case AST_DEREF: {
        ASTNode *operand = node->children[0];
        if (operand->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, operand->value);
            if (lv && lv->full_type && lv->kind == TYPE_POINTER &&
                lv->full_type->base) {
                return lv->full_type->base->kind;
            }
            GlobalVar *gv = codegen_find_global(cg, operand->value);
            if (gv && gv->full_type && gv->kind == TYPE_POINTER &&
                gv->full_type->base) {
                return gv->full_type->base->kind;
            }
        }
        return TYPE_INT;
    }
    case AST_ARRAY_INDEX: {
        ASTNode *base = node->children[0];
        if (base->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, base->value);
            if (lv && lv->full_type &&
                (lv->kind == TYPE_ARRAY || lv->kind == TYPE_POINTER) &&
                lv->full_type->base) {
                return lv->full_type->base->kind;
            }
            GlobalVar *gv = codegen_find_global(cg, base->value);
            if (gv && gv->full_type &&
                (gv->kind == TYPE_ARRAY || gv->kind == TYPE_POINTER) &&
                gv->full_type->base) {
                return gv->full_type->base->kind;
            }
        }
        return TYPE_INT;
    }
    case AST_BINARY_OP: {
        const char *op = node->value;
        if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
            strcmp(op, "*") == 0 || strcmp(op, "/") == 0 ||
            strcmp(op, "%") == 0 || strcmp(op, "&") == 0 ||
            strcmp(op, "^") == 0 || strcmp(op, "|") == 0) {
            TypeKind lt = sizeof_type_of_expr(cg, node->children[0]);
            TypeKind rt = sizeof_type_of_expr(cg, node->children[1]);
            return common_arith_kind(promote_kind(lt), promote_kind(rt));
        }
        if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
            return promote_kind(sizeof_type_of_expr(cg, node->children[0]));
        }
        return TYPE_INT; /* comparisons, &&, || */
    }
    case AST_UNARY_OP: {
        const char *op = node->value;
        if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
            strcmp(op, "~") == 0) {
            return promote_kind(sizeof_type_of_expr(cg, node->children[0]));
        }
        return TYPE_INT; /* ! */
    }
    case AST_ADDR_OF:
        return TYPE_POINTER;
    case AST_CAST:
        return node->decl_type;
    case AST_FUNCTION_CALL:
        return node->decl_type;
    case AST_INDIRECT_CALL:
        return node->decl_type ? node->decl_type : TYPE_INT;
    case AST_PREFIX_INC_DEC:
    case AST_POSTFIX_INC_DEC:
        return sizeof_type_of_expr(cg, node->children[0]);
    case AST_ASSIGNMENT: {
        if (node->value[0] != '\0') {
            LocalVar *lv = codegen_find_var(cg, node->value);
            if (lv) return lv->kind;
            GlobalVar *gv = codegen_find_global(cg, node->value);
            if (gv) return gv->kind;
        } else if (node->child_count > 0) {
            return sizeof_type_of_expr(cg, node->children[0]);
        }
        return TYPE_INT;
    }
    case AST_COMMA_EXPR:
        return sizeof_type_of_expr(cg, node->children[1]);
    case AST_MEMBER_ACCESS: {
        ASTNode *base = node->children[0];
        if (base->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, base->value);
            if (lv && (lv->kind == TYPE_STRUCT || lv->kind == TYPE_UNION) &&
                lv->full_type) {
                StructField *f = find_struct_field(lv->full_type, node->value);
                if (f) return f->kind;
            }
        }
        /* Stage 34: (*ptr).field */
        if (base->type == AST_DEREF && base->child_count > 0 &&
            base->children[0]->type == AST_VAR_REF) {
            LocalVar *plv = codegen_find_var(cg, base->children[0]->value);
            if (plv && plv->kind == TYPE_POINTER && plv->full_type &&
                plv->full_type->base &&
                (plv->full_type->base->kind == TYPE_STRUCT ||
                 plv->full_type->base->kind == TYPE_UNION)) {
                StructField *f = find_struct_field(plv->full_type->base, node->value);
                if (f) return f->kind;
            }
        }
        return TYPE_INT;
    }
    case AST_ARROW_ACCESS: {
        ASTNode *base = node->children[0];
        if (base->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, base->value);
            if (lv && lv->kind == TYPE_POINTER && lv->full_type &&
                lv->full_type->base &&
                (lv->full_type->base->kind == TYPE_STRUCT ||
                 lv->full_type->base->kind == TYPE_UNION)) {
                StructField *f = find_struct_field(lv->full_type->base, node->value);
                if (f) return f->kind;
            }
        }
        return TYPE_INT;
    }
    default:
        return TYPE_INT;
    }
}

/*
 * Compute the result type of an expression and record it on the node.
 * Stage 11-03 tracks this for the operators brought into scope:
 * literals, identifiers, unary +/-, binary +/-/·//, and assignment.
 * Stage 11-05-02 adds function-call expressions, whose type is the
 * callee's declared return type (recorded on `decl_type` by the
 * parser). Operators that remain 32-bit in this stage (comparisons,
 * logical, inc/dec) report TYPE_INT so callers keep the 32-bit path.
 */
static TypeKind expr_result_type(CodeGen *cg, ASTNode *node) {
    if (!node) return TYPE_INT;
    TypeKind t = TYPE_INT;
    switch (node->type) {
    case AST_INT_LITERAL:
        t = node->decl_type;
        break;
    case AST_CHAR_LITERAL:
        t = TYPE_INT;
        break;
    case AST_VAR_REF: {
        LocalVar *lv = codegen_find_var(cg, node->value);
        if (lv) {
            if (lv->kind == TYPE_POINTER || lv->kind == TYPE_ARRAY) {
                /* Stage 13-03: an array name in a value context decays to
                 * a pointer to its element type. */
                t = TYPE_POINTER;
            } else {
                t = promote_kind(type_kind_from_size(lv->size));
            }
        } else {
            GlobalVar *gv = codegen_find_global(cg, node->value);
            if (gv && (gv->kind == TYPE_POINTER || gv->kind == TYPE_ARRAY)) {
                t = TYPE_POINTER;
            } else {
                t = gv ? promote_kind(type_kind_from_size(gv->size)) : TYPE_INT;
            }
        }
        break;
    }
    case AST_ADDR_OF:
        t = TYPE_POINTER;
        break;
    case AST_UNARY_OP:
        if (strcmp(node->value, "+") == 0 ||
            strcmp(node->value, "-") == 0 ||
            strcmp(node->value, "~") == 0) {
            t = promote_kind(expr_result_type(cg, node->children[0]));
        } else {
            t = TYPE_INT; /* ! stays 32-bit */
        }
        break;
    case AST_BINARY_OP: {
        const char *op = node->value;
        if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
            strcmp(op, "*") == 0 || strcmp(op, "/") == 0 ||
            strcmp(op, "%") == 0) {
            TypeKind lt = expr_result_type(cg, node->children[0]);
            TypeKind rt = expr_result_type(cg, node->children[1]);
            /* Stage 13-04: pointer arithmetic — `T* +/- int` and
             * `int + T*` produce a pointer. Validation of the
             * specific combinations happens in codegen. */
            if ((strcmp(op, "+") == 0 || strcmp(op, "-") == 0) &&
                (lt == TYPE_POINTER || rt == TYPE_POINTER)) {
                t = TYPE_POINTER;
            } else {
                t = common_arith_kind(lt, rt);
            }
        } else if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
            /* Stage 16-03: shift result type follows the promoted
             * type of the LEFT operand only. The right operand is a
             * shift count and does not participate. */
            t = promote_kind(expr_result_type(cg, node->children[0]));
        } else if (strcmp(op, "&") == 0 || strcmp(op, "^") == 0 ||
                   strcmp(op, "|") == 0) {
            /* Stage 16-04: bitwise binary operators use the standard
             * common-arithmetic-type rule (char/short/int → int;
             * either side long → long). Pointer operands are
             * rejected at codegen. */
            TypeKind lt = expr_result_type(cg, node->children[0]);
            TypeKind rt = expr_result_type(cg, node->children[1]);
            t = common_arith_kind(lt, rt);
        } else {
            t = TYPE_INT; /* comparisons, && , || stay 32-bit */
        }
        break;
    }
    case AST_ASSIGNMENT: {
        LocalVar *lv = codegen_find_var(cg, node->value);
        t = lv ? type_kind_from_size(lv->size) : TYPE_INT;
        break;
    }
    case AST_FUNCTION_CALL:
        t = node->decl_type;
        break;
    case AST_INDIRECT_CALL:
        t = node->decl_type ? node->decl_type : TYPE_INT;
        break;
    case AST_CAST:
        t = node->decl_type;
        break;
    case AST_ARRAY_INDEX: {
        /* The result is the element type, promoted to int for
         * char/short. Pointer elements stay TYPE_POINTER. The base
         * may be an array local (Stage 13-02) or a pointer local
         * (Stage 13-03). */
        ASTNode *base_node = node->children[0];
        if (base_node->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, base_node->value);
            if (lv && lv->full_type &&
                (lv->kind == TYPE_ARRAY || lv->kind == TYPE_POINTER)) {
                Type *element = lv->full_type->base;
                if (element->kind == TYPE_POINTER) {
                    t = TYPE_POINTER;
                } else {
                    t = promote_kind(type_kind_from_size(element->size));
                }
            }
        }
        break;
    }
    case AST_SIZEOF_TYPE:
    case AST_SIZEOF_EXPR:
        t = TYPE_LONG;
        break;
    case AST_COMMA_EXPR:
        t = expr_result_type(cg, node->children[1]);
        break;
    case AST_MEMBER_ACCESS: {
        ASTNode *base_node = node->children[0];
        if (base_node->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, base_node->value);
            if (lv && (lv->kind == TYPE_STRUCT || lv->kind == TYPE_UNION) &&
                lv->full_type) {
                StructField *f = find_struct_field(lv->full_type, node->value);
                if (f) {
                    t = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                        : promote_kind(f->kind);
                    if (f->kind == TYPE_POINTER) node->full_type = f->full_type;
                }
            }
        }
        /* Stage 34: (*ptr).field */
        if (base_node->type == AST_DEREF && base_node->child_count > 0 &&
            base_node->children[0]->type == AST_VAR_REF) {
            LocalVar *plv = codegen_find_var(cg, base_node->children[0]->value);
            if (plv && plv->kind == TYPE_POINTER && plv->full_type &&
                plv->full_type->base &&
                (plv->full_type->base->kind == TYPE_STRUCT ||
                 plv->full_type->base->kind == TYPE_UNION)) {
                StructField *f = find_struct_field(plv->full_type->base, node->value);
                if (f) {
                    t = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                        : promote_kind(f->kind);
                    if (f->kind == TYPE_POINTER) node->full_type = f->full_type;
                }
            }
        }
        break;
    }
    case AST_ARROW_ACCESS: {
        ASTNode *base_node = node->children[0];
        if (base_node->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, base_node->value);
            if (lv && lv->kind == TYPE_POINTER && lv->full_type &&
                lv->full_type->base &&
                (lv->full_type->base->kind == TYPE_STRUCT ||
                 lv->full_type->base->kind == TYPE_UNION)) {
                StructField *f = find_struct_field(lv->full_type->base, node->value);
                if (f) {
                    t = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                        : promote_kind(f->kind);
                    if (f->kind == TYPE_POINTER) node->full_type = f->full_type;
                }
            }
        }
        break;
    }
    default:
        t = TYPE_INT;
        break;
    }
    node->result_type = t;
    return t;
}

static void codegen_expression(CodeGen *cg, ASTNode *node) {
    if (node->type == AST_INT_LITERAL) {
        if (node->decl_type == TYPE_LONG ||
            node->decl_type == TYPE_LONG_LONG ||
            node->decl_type == TYPE_UNSIGNED_LONG_LONG) {
            fprintf(cg->output, "    mov rax, %s\n", node->value);
            node->result_type = TYPE_LONG;
        } else {
            fprintf(cg->output, "    mov eax, %s\n", node->value);
            node->result_type = TYPE_INT;
        }
        /* node->is_unsigned was set by the parser from the U/u suffix. */
        return;
    }
    if (node->type == AST_CHAR_LITERAL) {
        /* Stage 15-02: a character constant has type int. The decoded
         * byte sits at node->value[0]; emit the integer value
         * directly so downstream consumers (return, arithmetic, etc.)
         * see a 32-bit int in eax. */
        unsigned char b = (unsigned char)node->value[0];
        fprintf(cg->output, "    mov eax, %d\n", b);
        node->result_type = TYPE_INT;
        return;
    }
    if (node->type == AST_STRING_LITERAL) {
        /* Stage 14-03: register the literal in the per-translation-unit
         * string pool, assigning it the next `Lstr<N>` label, then emit
         * a PC-relative load of that label's address into rax. The
         * `.rodata` section is written once at the end of the unit
         * (see codegen_translation_unit).
         *
         * Stage 14-04: array-to-pointer decay. The literal's logical
         * type is `char[N+1]` but in an expression context it decays
         * to `char *` — set the result type accordingly so init,
         * assignment, comparison, and pointer arithmetic line up with
         * the same checks pointer locals receive. The byte payload
         * needed by the `.rodata` emitter is recovered from
         * `strlen(node->value)` since `full_type` no longer carries
         * the array length. */
        if (cg->string_pool_count >= MAX_STRING_LITERALS) {
            compile_error(
                    "error: too many string literals (max %d)\n",
                    MAX_STRING_LITERALS);
        }
        int idx = cg->string_pool_count;
        cg->string_pool[idx] = node;
        cg->string_pool_count++;
        fprintf(cg->output, "    lea rax, [rel Lstr%d]\n", idx);
        node->result_type = TYPE_POINTER;
        node->full_type = type_pointer(type_char());
        return;
    }
    if (node->type == AST_VAR_REF) {
        LocalVar *lv = codegen_find_var(cg, node->value);
        if (lv) {
            /* Stage 13-03: array name in a value context decays to a
             * pointer to its element type. The value is the array's base
             * address (lea), not a load from the slot. Whole-array
             * assignment is still rejected by the AST_ASSIGNMENT path,
             * which checks the LHS name before reaching this code. */
            if (lv->kind == TYPE_ARRAY) {
                fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset);
                node->result_type = TYPE_POINTER;
                node->full_type = type_pointer(lv->full_type->base);
                return;
            }
            if (lv->kind == TYPE_POINTER) {
                node->result_type = TYPE_POINTER;
                node->full_type = lv->full_type;
            } else {
                node->result_type = promote_kind(type_kind_from_size(lv->size));
                node->is_unsigned = lv->is_unsigned;
            }
            if (lv->is_static)
                emit_load_global(cg, lv->static_label, lv->size, lv->is_unsigned);
            else
                emit_load_local(cg, lv->offset, lv->size, lv->is_unsigned);
            return;
        }
        /* Stage 22-01: fall back to global table. */
        GlobalVar *gv = codegen_find_global(cg, node->value);
        if (!gv) {
            /* Stage 25-02: check if the name is a function designator. A
             * function name used as a value decays to a pointer to that
             * function type; emit its address with lea. */
            ASTNode *func_decl = codegen_find_function_decl(cg, node->value);
            if (func_decl) {
                fprintf(cg->output, "    lea rax, [rel %s]\n", node->value);
                node->result_type = TYPE_POINTER;
                node->full_type = build_func_designator_type(func_decl);
                return;
            }
            compile_error( "error: undeclared variable '%s'\n", node->value);
        }
        if (gv->kind == TYPE_ARRAY) {
            fprintf(cg->output, "    lea rax, [rel %s]\n", gv->name);
            node->result_type = TYPE_POINTER;
            node->full_type = type_pointer(gv->full_type->base);
            return;
        }
        if (gv->kind == TYPE_POINTER) {
            node->result_type = TYPE_POINTER;
            node->full_type = gv->full_type;
        } else {
            node->result_type = promote_kind(type_kind_from_size(gv->size));
            node->is_unsigned = gv->is_unsigned;
        }
        emit_load_global(cg, gv->name, gv->size, gv->is_unsigned);
        return;
    }
    if (node->type == AST_ASSIGNMENT) {
        /* Stage 13-02: array-element assignment `a[i] = rhs`. Compute
         * the element address, stash it, evaluate the RHS, convert it
         * to the element type, and store through the address with the
         * element type's width. Mirrors the deref-LHS path. */
        if (node->child_count == 2 &&
            node->children[0]->type == AST_ARRAY_INDEX) {
            Type *element = emit_array_index_addr(cg, node->children[0]);
            int sz = type_size(element);
            fprintf(cg->output, "    push rax\n");
            cg->push_depth++;
            codegen_expression(cg, node->children[1]);
            /* Stage 42: enforce pointer assignment rules for pointer-element arrays. */
            if (element->kind == TYPE_POINTER) {
                int rhs_is_null = is_null_pointer_constant(node->children[1]);
                TypeKind rhs_kind = node->children[1]->result_type;
                if (!rhs_is_null && rhs_kind != TYPE_POINTER) {
                    compile_error(
                            "error: assigning nonzero integer to pointer\n");
                }
                if (!rhs_is_null && rhs_kind == TYPE_POINTER &&
                    !pointer_types_assignable(element,
                                              node->children[1]->full_type)) {
                    compile_error(
                            "error: incompatible pointer type in array element assignment\n");
                }
            }
            emit_convert(cg, node->children[1]->result_type, element->kind);
            fprintf(cg->output, "    pop rbx\n");
            cg->push_depth--;
            switch (sz) {
            case 1: fprintf(cg->output, "    mov [rbx], al\n"); break;
            case 2: fprintf(cg->output, "    mov [rbx], ax\n"); break;
            case 8: fprintf(cg->output, "    mov [rbx], rax\n"); break;
            case 4:
            default: fprintf(cg->output, "    mov [rbx], eax\n"); break;
            }
            node->result_type = element->kind;
            if (element->kind == TYPE_POINTER) {
                node->full_type = element;
            }
            return;
        }
        /* Stage 31: struct member assignment via "." */
        if (node->child_count == 2 &&
            node->children[0]->type == AST_MEMBER_ACCESS) {
            StructField *f = emit_member_addr(cg, node->children[0]);
            int sz = f->full_type ? type_size(f->full_type) : 0;
            if (sz == 0) {
                switch (f->kind) {
                case TYPE_CHAR:  sz = 1; break;
                case TYPE_SHORT: sz = 2; break;
                case TYPE_LONG_LONG:
                case TYPE_UNSIGNED_LONG_LONG:
                case TYPE_LONG:
                case TYPE_POINTER: sz = 8; break;
                default: sz = 4; break;
                }
            }
            fprintf(cg->output, "    push rax\n");
            cg->push_depth++;
            codegen_expression(cg, node->children[1]);
            fprintf(cg->output, "    pop rbx\n");
            cg->push_depth--;
            switch (sz) {
            case 1: fprintf(cg->output, "    mov [rbx], al\n"); break;
            case 2: fprintf(cg->output, "    mov [rbx], ax\n"); break;
            case 8: fprintf(cg->output, "    mov [rbx], rax\n"); break;
            case 4:
            default: fprintf(cg->output, "    mov [rbx], eax\n"); break;
            }
            node->result_type = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                                : promote_kind(f->kind);
            return;
        }
        /* Stage 31: struct member assignment via "->" */
        if (node->child_count == 2 &&
            node->children[0]->type == AST_ARROW_ACCESS) {
            StructField *f = emit_arrow_addr(cg, node->children[0]);
            int sz = f->full_type ? type_size(f->full_type) : 0;
            if (sz == 0) {
                switch (f->kind) {
                case TYPE_CHAR:  sz = 1; break;
                case TYPE_SHORT: sz = 2; break;
                case TYPE_LONG_LONG:
                case TYPE_UNSIGNED_LONG_LONG:
                case TYPE_LONG:
                case TYPE_POINTER: sz = 8; break;
                default: sz = 4; break;
                }
            }
            fprintf(cg->output, "    push rax\n");
            cg->push_depth++;
            codegen_expression(cg, node->children[1]);
            fprintf(cg->output, "    pop rbx\n");
            cg->push_depth--;
            switch (sz) {
            case 1: fprintf(cg->output, "    mov [rbx], al\n"); break;
            case 2: fprintf(cg->output, "    mov [rbx], ax\n"); break;
            case 8: fprintf(cg->output, "    mov [rbx], rax\n"); break;
            case 4:
            default: fprintf(cg->output, "    mov [rbx], eax\n"); break;
            }
            node->result_type = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                                : promote_kind(f->kind);
            return;
        }
        /* Stage 12-03: deref-LHS assignment uses a different shape —
         * two children [AST_DEREF, RHS] and no `value`. Take the
         * pointer-store path: evaluate the pointer to an address,
         * stash it on the stack, evaluate the RHS, convert it to the
         * pointed-to base type, and store through the address with
         * the base type's width. */
        if (node->child_count == 2 &&
            node->children[0]->type == AST_DEREF) {
            ASTNode *deref = node->children[0];
            codegen_expression(cg, deref->children[0]);
            Type *operand_type = deref->children[0]->full_type;
            if (!operand_type || operand_type->kind != TYPE_POINTER) {
                compile_error( "error: cannot dereference non-pointer value\n");
            }
            Type *base = operand_type->base;
            /* Stage 66: reject assignment through a pointer-to-const type. */
            if (base && base->is_const) {
                compile_error(
                        "error: assignment through pointer to const-qualified type\n");
            }
            int sz = type_size(base);
            fprintf(cg->output, "    push rax\n");
            cg->push_depth++;
            codegen_expression(cg, node->children[1]);
            emit_convert(cg, node->children[1]->result_type, base->kind);
            fprintf(cg->output, "    pop rbx\n");
            cg->push_depth--;
            switch (sz) {
            case 1: fprintf(cg->output, "    mov [rbx], al\n"); break;
            case 2: fprintf(cg->output, "    mov [rbx], ax\n"); break;
            case 8: fprintf(cg->output, "    mov [rbx], rax\n"); break;
            case 4:
            default: fprintf(cg->output, "    mov [rbx], eax\n"); break;
            }
            node->result_type = base->kind;
            if (base->kind == TYPE_POINTER) {
                node->full_type = base;
            }
            return;
        }
        LocalVar *lv = codegen_find_var(cg, node->value);
        if (lv) {
            /* Stage 13-01: arrays are not assignable. */
            if (lv->kind == TYPE_ARRAY) {
                compile_error( "error: arrays are not assignable\n");
            }
            /* Stage 39: reject assignment to a const-qualified variable. */
            if (lv->is_const) {
                compile_error(
                        "error: assignment to const variable '%s'\n", lv->name);
            }
            /* Stage 33/72: struct/union-to-struct/union assignment — byte copy. */
            if ((lv->kind == TYPE_STRUCT || lv->kind == TYPE_UNION) && lv->full_type) {
                if (node->child_count < 1 || node->children[0]->type != AST_VAR_REF) {
                    compile_error( "error: %s assignment requires a %s variable\n",
                            type_kind_name(lv->kind), type_kind_name(lv->kind));
                }
                LocalVar *src = codegen_find_var(cg, node->children[0]->value);
                if (!src || src->kind != lv->kind || !src->full_type) {
                    compile_error( "error: cannot assign non-%s to %s '%s'\n",
                            type_kind_name(lv->kind), type_kind_name(lv->kind), lv->name);
                }
                if (src->full_type != lv->full_type) {
                    compile_error( "error: incompatible %s types in assignment to '%s'\n",
                            type_kind_name(lv->kind), lv->name);
                }
                int sz = lv->full_type->size;
                for (int b = 0; b < sz; b++) {
                    fprintf(cg->output, "    movzx eax, byte [rbp - %d]\n", src->offset - b);
                    fprintf(cg->output, "    mov [rbp - %d], al\n", lv->offset - b);
                }
                node->result_type = lv->kind;
                node->full_type = lv->full_type;
                return;
            }
            /* Stage 38: reject assigning a void function call result. */
            if (node->children[0]->type == AST_FUNCTION_CALL &&
                node->children[0]->decl_type == TYPE_VOID) {
                compile_error(
                        "error: cannot use void function result in assignment to '%s'\n",
                        lv->name);
            }
            codegen_expression(cg, node->children[0]);
            /* Stage 25-02: when the LHS is a function pointer, verify
             * the RHS carries a compatible function pointer type. */
            if (lv->kind == TYPE_POINTER && lv->full_type && lv->full_type->base &&
                lv->full_type->base->kind == TYPE_FUNCTION) {
                Type *rhs_type = node->children[0]->full_type;
                if (!rhs_type || rhs_type->kind != TYPE_POINTER || !rhs_type->base ||
                    rhs_type->base->kind != TYPE_FUNCTION ||
                    !func_ptr_types_equal_cg(lv->full_type, rhs_type)) {
                    compile_error(
                            "error: incompatible function pointer type in assignment to '%s'\n",
                            lv->name);
                }
            }
            /* Stage 66: warn when assigning a const-pointee pointer to a
             * non-const-pointee pointer (discards the const qualifier). */
            if (lv->kind == TYPE_POINTER && lv->full_type && lv->full_type->base &&
                !is_null_pointer_constant(node->children[0])) {
                Type *rhs_full = node->children[0]->full_type;
                if (rhs_full && rhs_full->kind == TYPE_POINTER && rhs_full->base &&
                    rhs_full->base->is_const && !lv->full_type->base->is_const) {
                    codegen_warn_const_discard(cg, "assignment to", lv->name);
                }
            }
            /* Pointer RHS values are already in full rax — skip the
             * 32-to-64 sign-extend that integer values would need. */
            int rhs_is_long = (node->children[0]->result_type == TYPE_LONG ||
                               node->children[0]->result_type == TYPE_POINTER);
            /* Stage 63: _Bool assignment normalizes any nonzero value to 1. */
            if (lv->kind == TYPE_BOOL)
                emit_bool_normalize(cg, rhs_is_long);
            if (lv->is_static)
                emit_store_global(cg, lv->static_label, lv->size, rhs_is_long);
            else
                emit_store_local(cg, lv->offset, lv->size, rhs_is_long);
            if (lv->kind == TYPE_POINTER) {
                node->result_type = TYPE_POINTER;
                node->full_type = lv->full_type;
            } else {
                node->result_type = type_kind_from_size(lv->size);
            }
            return;
        }
        /* Stage 22-01: fall back to global table. */
        GlobalVar *gv = codegen_find_global(cg, node->value);
        if (!gv) {
            compile_error( "error: undeclared variable '%s'\n", node->value);
        }
        if (gv->kind == TYPE_ARRAY) {
            compile_error( "error: arrays are not assignable\n");
        }
        /* Stage 39: reject assignment to a const-qualified global. */
        if (gv->is_const) {
            compile_error(
                    "error: assignment to const variable '%s'\n", gv->name);
        }
        /* Stage 38: reject assigning a void function call result. */
        if (node->children[0]->type == AST_FUNCTION_CALL &&
            node->children[0]->decl_type == TYPE_VOID) {
            compile_error(
                    "error: cannot use void function result in assignment to '%s'\n",
                    gv->name);
        }
        codegen_expression(cg, node->children[0]);
        /* Stage 25-02: same function pointer type check for global LHS. */
        if (gv->kind == TYPE_POINTER && gv->full_type && gv->full_type->base &&
            gv->full_type->base->kind == TYPE_FUNCTION) {
            Type *rhs_type = node->children[0]->full_type;
            if (!rhs_type || rhs_type->kind != TYPE_POINTER || !rhs_type->base ||
                rhs_type->base->kind != TYPE_FUNCTION ||
                !func_ptr_types_equal_cg(gv->full_type, rhs_type)) {
                compile_error(
                        "error: incompatible function pointer type in assignment to '%s'\n",
                        gv->name);
            }
        }
        /* Stage 66: warn when global pointer assignment discards const. */
        if (gv->kind == TYPE_POINTER && gv->full_type && gv->full_type->base &&
            !is_null_pointer_constant(node->children[0])) {
            Type *rhs_full = node->children[0]->full_type;
            if (rhs_full && rhs_full->kind == TYPE_POINTER && rhs_full->base &&
                rhs_full->base->is_const && !gv->full_type->base->is_const) {
                codegen_warn_const_discard(cg, "assignment to", gv->name);
            }
        }
        int rhs_is_long_g = (node->children[0]->result_type == TYPE_LONG ||
                             node->children[0]->result_type == TYPE_POINTER);
        /* Stage 63: _Bool assignment normalizes any nonzero value to 1. */
        if (gv->kind == TYPE_BOOL)
            emit_bool_normalize(cg, rhs_is_long_g);
        emit_store_global(cg, gv->name, gv->size, rhs_is_long_g);
        if (gv->kind == TYPE_POINTER) {
            node->result_type = TYPE_POINTER;
            node->full_type = gv->full_type;
        } else {
            node->result_type = type_kind_from_size(gv->size);
        }
        return;
    }
    if (node->type == AST_ADDR_OF) {
        /* Operand is AST_VAR_REF or AST_ARRAY_INDEX (parser-enforced).
         * For a var-ref, take the variable's address with `lea`. For
         * an array subscript, reuse the array-index address helper:
         * `&a[i]` evaluates `a + i * sizeof(*a)` without loading
         * through it. The result type is pointer-to-element in both
         * cases. */
        ASTNode *operand = node->children[0];
        if (operand->type == AST_ARRAY_INDEX) {
            Type *element = emit_array_index_addr(cg, operand);
            node->result_type = TYPE_POINTER;
            node->full_type = type_pointer(element);
            return;
        }
        LocalVar *lv = codegen_find_var(cg, operand->value);
        if (lv) {
            if (lv->is_static)
                fprintf(cg->output, "    lea rax, [rel %s]\n", lv->static_label);
            else
                fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset);
            node->result_type = TYPE_POINTER;
            node->full_type = type_pointer(local_var_type(lv));
            return;
        }
        GlobalVar *gv = codegen_find_global(cg, operand->value);
        if (!gv) {
            compile_error( "error: undeclared variable '%s'\n", operand->value);
        }
        fprintf(cg->output, "    lea rax, [rel %s]\n", gv->name);
        node->result_type = TYPE_POINTER;
        node->full_type = type_pointer(global_var_type(gv));
        return;
    }
    if (node->type == AST_DEREF) {
        /* Evaluate the pointer expression — its value (the address)
         * lands in rax. Operand must be of pointer type; load through
         * the address using the base type's width. */
        codegen_expression(cg, node->children[0]);
        Type *operand_type = node->children[0]->full_type;
        if (!operand_type || operand_type->kind != TYPE_POINTER) {
            compile_error( "error: cannot dereference non-pointer value\n");
        }
        Type *base = operand_type->base;
        /* Stage 38: reject dereferencing a void pointer. */
        if (base->kind == TYPE_VOID) {
            compile_error( "error: cannot dereference void pointer\n");
        }
        /* Stage 25-03: dereferencing a function pointer is an identity —
         * the address already in rax is the callable value; no memory
         * load.  The result type is TYPE_POINTER so the indirect-call
         * handler can validate it as a function pointer. */
        if (base->kind == TYPE_FUNCTION) {
            node->result_type = TYPE_POINTER;
            node->full_type = operand_type;
            return;
        }
        int sz = type_size(base);
        switch (sz) {
        case 1: fprintf(cg->output, "    movsx eax, byte [rax]\n"); break;
        case 2: fprintf(cg->output, "    movsx eax, word [rax]\n"); break;
        case 8: fprintf(cg->output, "    mov rax, [rax]\n"); break;
        case 4:
        default: fprintf(cg->output, "    mov eax, [rax]\n"); break;
        }
        node->result_type = base->kind;
        if (base->kind == TYPE_POINTER) {
            node->full_type = base;
        }
        return;
    }
    if (node->type == AST_ARRAY_INDEX) {
        /* Stage 13-02: array subscript read. Compute the element's
         * address and load through it using the element type's width.
         * The result is the element value, sign-extended into eax/rax
         * for char/short and loaded directly for int/long/pointer. */
        Type *element = emit_array_index_addr(cg, node);
        int sz = type_size(element);
        switch (sz) {
        case 1: fprintf(cg->output, "    movsx eax, byte [rax]\n"); break;
        case 2: fprintf(cg->output, "    movsx eax, word [rax]\n"); break;
        case 8: fprintf(cg->output, "    mov rax, [rax]\n"); break;
        case 4:
        default: fprintf(cg->output, "    mov eax, [rax]\n"); break;
        }
        node->result_type = element->kind;
        if (element->kind == TYPE_POINTER) {
            node->full_type = element;
        }
        return;
    }
    if (node->type == AST_MEMBER_ACCESS) {
        StructField *f = emit_member_addr(cg, node);
        int sz = type_size(f->full_type ? f->full_type : NULL);
        if (sz == 0) {
            /* scalar field — derive size from kind */
            switch (f->kind) {
            case TYPE_CHAR:  sz = 1; break;
            case TYPE_SHORT: sz = 2; break;
            case TYPE_LONG_LONG:
            case TYPE_UNSIGNED_LONG_LONG:
            case TYPE_LONG:
            case TYPE_POINTER: sz = 8; break;
            default: sz = 4; break;
            }
        }
        switch (sz) {
        case 1: fprintf(cg->output, "    movsx eax, byte [rax]\n"); break;
        case 2: fprintf(cg->output, "    movsx eax, word [rax]\n"); break;
        case 8: fprintf(cg->output, "    mov rax, [rax]\n"); break;
        case 4:
        default: fprintf(cg->output, "    mov eax, [rax]\n"); break;
        }
        node->result_type = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                            : promote_kind(f->kind);
        if (f->kind == TYPE_POINTER) node->full_type = f->full_type;
        return;
    }
    if (node->type == AST_ARROW_ACCESS) {
        StructField *f = emit_arrow_addr(cg, node);
        int sz = f->full_type ? type_size(f->full_type) : 0;
        if (sz == 0) {
            switch (f->kind) {
            case TYPE_CHAR:  sz = 1; break;
            case TYPE_SHORT: sz = 2; break;
            case TYPE_LONG_LONG:
            case TYPE_UNSIGNED_LONG_LONG:
            case TYPE_LONG:
            case TYPE_POINTER: sz = 8; break;
            default: sz = 4; break;
            }
        }
        switch (sz) {
        case 1: fprintf(cg->output, "    movsx eax, byte [rax]\n"); break;
        case 2: fprintf(cg->output, "    movsx eax, word [rax]\n"); break;
        case 8: fprintf(cg->output, "    mov rax, [rax]\n"); break;
        case 4:
        default: fprintf(cg->output, "    mov eax, [rax]\n"); break;
        }
        node->result_type = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                            : promote_kind(f->kind);
        if (f->kind == TYPE_POINTER) node->full_type = f->full_type;
        return;
    }
    if (node->type == AST_SIZEOF_TYPE) {
        int sz;
        if ((node->decl_type == TYPE_STRUCT || node->decl_type == TYPE_UNION) &&
            node->full_type) {
            if (node->full_type->size == 0) {
                compile_error( "error: sizeof applied to incomplete %s type\n",
                        type_kind_name(node->decl_type));
            }
            sz = node->full_type->size;
        } else {
            switch (node->decl_type) {
            case TYPE_BOOL:               sz = 1; break;
            case TYPE_CHAR:               sz = 1; break;
            case TYPE_SHORT:              sz = 2; break;
            case TYPE_LONG:               sz = 8; break;
            case TYPE_LONG_LONG:          sz = 8; break;
            case TYPE_UNSIGNED_LONG_LONG: sz = 8; break;
            case TYPE_POINTER:            sz = 8; break;
            default:                      sz = 4; break; /* TYPE_INT */
            }
        }
        fprintf(cg->output, "    mov rax, %d\n", sz);
        node->result_type = TYPE_LONG;
        return;
    }
    if (node->type == AST_SIZEOF_EXPR) {
        ASTNode *child = node->children[0];
        if (child->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, child->value);
            if (lv && lv->kind == TYPE_ARRAY && lv->full_type) {
                fprintf(cg->output, "    mov rax, %d\n", lv->full_type->size);
                node->result_type = TYPE_LONG;
                return;
            }
            if (lv && (lv->kind == TYPE_STRUCT || lv->kind == TYPE_UNION) &&
                lv->full_type) {
                fprintf(cg->output, "    mov rax, %d\n", lv->full_type->size);
                node->result_type = TYPE_LONG;
                return;
            }
            if (!lv) {
                GlobalVar *gv = codegen_find_global(cg, child->value);
                if (gv && gv->kind == TYPE_ARRAY && gv->full_type) {
                    fprintf(cg->output, "    mov rax, %d\n", gv->full_type->size);
                    node->result_type = TYPE_LONG;
                    return;
                }
                if (gv && (gv->kind == TYPE_STRUCT || gv->kind == TYPE_UNION) &&
                    gv->full_type) {
                    fprintf(cg->output, "    mov rax, %d\n", gv->full_type->size);
                    node->result_type = TYPE_LONG;
                    return;
                }
            }
        }
        TypeKind kind = sizeof_type_of_expr(cg, child);
        int sz;
        switch (kind) {
        case TYPE_CHAR:               sz = 1; break;
        case TYPE_SHORT:              sz = 2; break;
        case TYPE_LONG:               sz = 8; break;
        case TYPE_LONG_LONG:          sz = 8; break;
        case TYPE_UNSIGNED_LONG_LONG: sz = 8; break;
        case TYPE_POINTER:            sz = 8; break;
        default:                      sz = 4; break; /* TYPE_INT */
        }
        fprintf(cg->output, "    mov rax, %d\n", sz);
        node->result_type = TYPE_LONG;
        return;
    }
    if (node->type == AST_UNARY_OP) {
        codegen_expression(cg, node->children[0]);
        const char *op = node->value;
        if (strcmp(op, "-") == 0) {
            TypeKind ot = promote_kind(node->children[0]->result_type);
            if (ot == TYPE_LONG) {
                fprintf(cg->output, "    neg rax\n");
            } else {
                fprintf(cg->output, "    neg eax\n");
            }
            node->result_type = ot;
        } else if (strcmp(op, "!") == 0) {
            if (node->children[0]->result_type == TYPE_POINTER) {
                compile_error(
                        "error: operator '!' not supported on pointer operands\n");
            }
            if (node->children[0]->result_type == TYPE_LONG) {
                fprintf(cg->output, "    cmp rax, 0\n");
            } else {
                fprintf(cg->output, "    cmp eax, 0\n");
            }
            fprintf(cg->output, "    sete al\n");
            fprintf(cg->output, "    movzx eax, al\n");
            node->result_type = TYPE_INT;
        } else if (strcmp(op, "~") == 0) {
            if (node->children[0]->result_type == TYPE_POINTER) {
                compile_error(
                        "error: operator '~' not supported on pointer operands\n");
            }
            TypeKind ot = promote_kind(node->children[0]->result_type);
            if (ot == TYPE_LONG) {
                fprintf(cg->output, "    not rax\n");
            } else {
                fprintf(cg->output, "    not eax\n");
            }
            node->result_type = ot;
        } else {
            /* unary + is a no-op; promoted type propagates */
            node->result_type = promote_kind(node->children[0]->result_type);
        }
        return;
    }
    if (node->type == AST_PREFIX_INC_DEC) {
        /* ++x or --x: update variable, result is new value.
         * Stage 41: pointers scale by sizeof(*p) and use 64-bit ops. */
        const char *var_name = node->children[0]->value;
        LocalVar *lv = codegen_find_var(cg, var_name);
        if (lv) {
            if (lv->is_static)
                emit_load_global(cg, lv->static_label, lv->size, lv->is_unsigned);
            else
                emit_load_local(cg, lv->offset, lv->size, lv->is_unsigned);
            if (lv->kind == TYPE_POINTER && lv->full_type) {
                int elem_size = type_size(lv->full_type->base);
                if (strcmp(node->value, "++") == 0) {
                    fprintf(cg->output, "    add rax, %d\n", elem_size);
                } else {
                    fprintf(cg->output, "    sub rax, %d\n", elem_size);
                }
                if (lv->is_static)
                    emit_store_global(cg, lv->static_label, lv->size, 1);
                else
                    emit_store_local(cg, lv->offset, lv->size, 1);
                node->result_type = TYPE_POINTER;
                node->full_type = lv->full_type;
            } else {
                if (strcmp(node->value, "++") == 0) {
                    fprintf(cg->output, "    add eax, 1\n");
                } else {
                    fprintf(cg->output, "    sub eax, 1\n");
                }
                if (lv->is_static)
                    emit_store_global(cg, lv->static_label, lv->size, 0);
                else
                    emit_store_local(cg, lv->offset, lv->size, 0);
                node->result_type = TYPE_INT;
            }
        } else {
            GlobalVar *gv = codegen_find_global(cg, var_name);
            if (!gv) {
                compile_error( "error: undeclared variable '%s'\n", var_name);
            }
            emit_load_global(cg, gv->name, gv->size, gv->is_unsigned);
            if (gv->kind == TYPE_POINTER && gv->full_type) {
                int elem_size = type_size(gv->full_type->base);
                if (strcmp(node->value, "++") == 0) {
                    fprintf(cg->output, "    add rax, %d\n", elem_size);
                } else {
                    fprintf(cg->output, "    sub rax, %d\n", elem_size);
                }
                emit_store_global(cg, gv->name, gv->size, 1);
                node->result_type = TYPE_POINTER;
                node->full_type = gv->full_type;
            } else {
                if (strcmp(node->value, "++") == 0) {
                    fprintf(cg->output, "    add eax, 1\n");
                } else {
                    fprintf(cg->output, "    sub eax, 1\n");
                }
                emit_store_global(cg, gv->name, gv->size, 0);
                node->result_type = TYPE_INT;
            }
        }
        return;
    }
    if (node->type == AST_POSTFIX_INC_DEC) {
        /* x++ or x--: result is old value, then update variable.
         * Stage 41: pointers scale by sizeof(*p) and use 64-bit ops. */
        const char *var_name = node->children[0]->value;
        LocalVar *lv = codegen_find_var(cg, var_name);
        if (lv) {
            if (lv->is_static)
                emit_load_global(cg, lv->static_label, lv->size, lv->is_unsigned);
            else
                emit_load_local(cg, lv->offset, lv->size, lv->is_unsigned);
            if (lv->kind == TYPE_POINTER && lv->full_type) {
                int elem_size = type_size(lv->full_type->base);
                fprintf(cg->output, "    mov rcx, rax\n");  /* save old pointer */
                if (strcmp(node->value, "++") == 0) {
                    fprintf(cg->output, "    add rax, %d\n", elem_size);
                } else {
                    fprintf(cg->output, "    sub rax, %d\n", elem_size);
                }
                if (lv->is_static)
                    emit_store_global(cg, lv->static_label, lv->size, 1);
                else
                    emit_store_local(cg, lv->offset, lv->size, 1);
                fprintf(cg->output, "    mov rax, rcx\n");  /* result: old pointer */
                node->result_type = TYPE_POINTER;
                node->full_type = lv->full_type;
            } else {
                fprintf(cg->output, "    mov ecx, eax\n");  /* save old value */
                if (strcmp(node->value, "++") == 0) {
                    fprintf(cg->output, "    add eax, 1\n");
                } else {
                    fprintf(cg->output, "    sub eax, 1\n");
                }
                if (lv->is_static)
                    emit_store_global(cg, lv->static_label, lv->size, 0);
                else
                    emit_store_local(cg, lv->offset, lv->size, 0);
                fprintf(cg->output, "    mov eax, ecx\n");  /* result: old value */
                node->result_type = TYPE_INT;
            }
        } else {
            GlobalVar *gv = codegen_find_global(cg, var_name);
            if (!gv) {
                compile_error( "error: undeclared variable '%s'\n", var_name);
            }
            emit_load_global(cg, gv->name, gv->size, gv->is_unsigned);
            if (gv->kind == TYPE_POINTER && gv->full_type) {
                int elem_size = type_size(gv->full_type->base);
                fprintf(cg->output, "    mov rcx, rax\n");  /* save old pointer */
                if (strcmp(node->value, "++") == 0) {
                    fprintf(cg->output, "    add rax, %d\n", elem_size);
                } else {
                    fprintf(cg->output, "    sub rax, %d\n", elem_size);
                }
                emit_store_global(cg, gv->name, gv->size, 1);
                fprintf(cg->output, "    mov rax, rcx\n");  /* result: old pointer */
                node->result_type = TYPE_POINTER;
                node->full_type = gv->full_type;
            } else {
                fprintf(cg->output, "    mov ecx, eax\n");  /* save old value */
                if (strcmp(node->value, "++") == 0) {
                    fprintf(cg->output, "    add eax, 1\n");
                } else {
                    fprintf(cg->output, "    sub eax, 1\n");
                }
                emit_store_global(cg, gv->name, gv->size, 0);
                fprintf(cg->output, "    mov eax, ecx\n");  /* result: old value */
                node->result_type = TYPE_INT;
            }
        }
        return;
    }
    if (node->type == AST_CAST) {
        /* Evaluate the source expression, then apply the widen/narrow
         * conversion used by assignment, arg-passing and return paths.
         * The cast's result type is its declared target type. */
        codegen_expression(cg, node->children[0]);
        TypeKind src = node->children[0]->result_type;
        TypeKind dst = node->decl_type;
        emit_convert(cg, src, dst);
        node->result_type = dst;
        return;
    }
    if (node->type == AST_FUNCTION_CALL) {
        static const char *arg_regs[6] = {
            "rdi", "rsi", "rdx", "rcx", "r8", "r9"
        };
        int nargs = node->child_count;
        ASTNode *callee = codegen_find_function_decl(cg, node->value);

/* Emit type conversion for argument i against its declared parameter type. */
#define EMIT_ARG_CONVERT(call_node, callee_node, i) do { \
    if ((callee_node) && (i) < (callee_node)->child_count && \
        (callee_node)->children[(i)]->type == AST_PARAM) { \
        ASTNode *_p = (callee_node)->children[(i)]; \
        TypeKind _src = (call_node)->children[(i)]->result_type; \
        TypeKind _dst = _p->decl_type; \
        int _null_const = is_null_pointer_constant((call_node)->children[(i)]); \
        if (_dst == TYPE_POINTER || _src == TYPE_POINTER) { \
            if (_dst != TYPE_POINTER) { \
                compile_error( \
                        "error: function '%s' parameter '%s' expected integer argument, got pointer\n", \
                        (call_node)->value, _p->value); \
            } \
            if (_src != TYPE_POINTER && !_null_const) { \
                compile_error( \
                        "error: function '%s' parameter '%s' expected pointer argument, got integer\n", \
                        (call_node)->value, _p->value); \
            } \
            if (!_null_const && \
                    !pointer_types_assignable(_p->full_type, (call_node)->children[(i)]->full_type)) { \
                compile_error( \
                        "error: function '%s' parameter '%s' has incompatible pointer type\n", \
                        (call_node)->value, _p->value); \
            } \
        } else { \
            emit_convert(cg, _src, _dst); \
        } \
    } \
} while (0)

        if (nargs <= 6) {
            /* Evaluate all args left-to-right, push, then pop into regs. */
            for (int i = 0; i < nargs; i++) {
                codegen_expression(cg, node->children[i]);
                EMIT_ARG_CONVERT(node, callee, i);
                fprintf(cg->output, "    push rax\n");
                cg->push_depth++;
            }
            for (int i = nargs - 1; i >= 0; i--) {
                fprintf(cg->output, "    pop %s\n", arg_regs[i]);
                cg->push_depth--;
            }
            int needs_pad = (cg->push_depth % 2) != 0;
            if (needs_pad)
                fprintf(cg->output, "    sub rsp, 8\n");
            if (callee && callee->is_variadic)
                fprintf(cg->output, "    xor eax, eax\n");
            fprintf(cg->output, "    call %s\n", node->value);
            if (needs_pad)
                fprintf(cg->output, "    add rsp, 8\n");
        } else {
            /* Stage 68: N > 6 args — SysV AMD64 stack-passing protocol.
             * Alignment padding (if needed) goes deepest; then stack args
             * are pushed right-to-left (argN-1 first, arg6 last) so arg6
             * lands at the top of the stack and the callee finds it at
             * [rbp+16].  Register args are pushed after, then popped. */
            int num_stack = nargs - 6;
            int pad = ((cg->push_depth + num_stack) % 2) != 0 ? 1 : 0;
            int cleanup = (num_stack + pad) * 8;

            if (pad) {
                fprintf(cg->output, "    sub rsp, 8\n");
                cg->push_depth++;
            }
            /* Push stack args right-to-left. */
            for (int i = nargs - 1; i >= 6; i--) {
                codegen_expression(cg, node->children[i]);
                EMIT_ARG_CONVERT(node, callee, i);
                fprintf(cg->output, "    push rax\n");
                cg->push_depth++;
            }
            /* Push register args left-to-right. */
            for (int i = 0; i < 6; i++) {
                codegen_expression(cg, node->children[i]);
                EMIT_ARG_CONVERT(node, callee, i);
                fprintf(cg->output, "    push rax\n");
                cg->push_depth++;
            }
            /* Pop register args into rdi..r9. */
            for (int i = 5; i >= 0; i--) {
                fprintf(cg->output, "    pop %s\n", arg_regs[i]);
                cg->push_depth--;
            }
            if (callee && callee->is_variadic)
                fprintf(cg->output, "    xor eax, eax\n");
            fprintf(cg->output, "    call %s\n", node->value);
            fprintf(cg->output, "    add rsp, %d\n", cleanup);
            cg->push_depth -= (num_stack + pad);
        }
#undef EMIT_ARG_CONVERT

        node->result_type = node->decl_type;
        if (callee && callee->decl_type == TYPE_POINTER)
            node->full_type = callee->full_type;
        return;
    }
    if (node->type == AST_INDIRECT_CALL) {
        /* Stage 25-03: indirect call through a function pointer.
         * children[0] = callee expression (VAR_REF or DEREF of a fp);
         * children[1..n] = arguments. */
        static const char *arg_regs[6] = {
            "rdi", "rsi", "rdx", "rcx", "r8", "r9"
        };
        int nargs = node->child_count - 1;
        /* Pre-validate VAR_REF callee: if the name is not a known variable
         * (local or global), it was not declared as a function pointer before
         * this call site — reject with the same message as a direct call to
         * an undeclared function. */
        if (node->children[0]->type == AST_VAR_REF) {
            const char *callee_name = node->children[0]->value;
            LocalVar *clv = codegen_find_var(cg, callee_name);
            GlobalVar *cgv = clv ? NULL : codegen_find_global(cg, callee_name);
            if (!clv && !cgv) {
                compile_error( "error: call to undefined function '%s'\n",
                        callee_name);
            }
        }
        /* Evaluate callee first so we can validate its type, then save
         * the function address before clobbering rax with arguments. */
        codegen_expression(cg, node->children[0]);
        if (node->children[0]->result_type != TYPE_POINTER ||
            !node->children[0]->full_type ||
            !node->children[0]->full_type->base ||
            node->children[0]->full_type->base->kind != TYPE_FUNCTION) {
            compile_error( "error: expression is not callable\n");
        }
        Type *fn = node->children[0]->full_type->base; /* TYPE_FUNCTION node */
        if (fn->param_count != nargs) {
            compile_error(
                    "error: indirect call expects %d arguments, got %d\n",
                    fn->param_count, nargs);
        }
        /* Save callee address below the arguments. */
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;

/* Emit type conversion for indirect-call argument i. */
#define EMIT_INDIRECT_ARG_CONVERT(fn_type, call_node, i) do { \
    TypeKind _src = (call_node)->children[(i) + 1]->result_type; \
    TypeKind _dst = (fn_type)->params[(i)]->kind; \
    if (_dst == TYPE_POINTER || _src == TYPE_POINTER) { \
        if (_dst != TYPE_POINTER) { \
            compile_error( \
                    "error: indirect call argument %d: expected integer, got pointer\n", \
                    (i) + 1); \
        } \
        if (_src != TYPE_POINTER) { \
            compile_error( \
                    "error: indirect call argument %d: expected pointer, got integer\n", \
                    (i) + 1); \
        } \
    } else { \
        emit_convert(cg, _src, _dst); \
    } \
} while (0)

        if (nargs <= 6) {
            for (int i = 0; i < nargs; i++) {
                codegen_expression(cg, node->children[i + 1]);
                EMIT_INDIRECT_ARG_CONVERT(fn, node, i);
                fprintf(cg->output, "    push rax\n");
                cg->push_depth++;
            }
            for (int i = nargs - 1; i >= 0; i--) {
                fprintf(cg->output, "    pop %s\n", arg_regs[i]);
                cg->push_depth--;
            }
            fprintf(cg->output, "    pop r10\n");
            cg->push_depth--;
            int needs_pad = (cg->push_depth % 2) != 0;
            if (needs_pad) fprintf(cg->output, "    sub rsp, 8\n");
            fprintf(cg->output, "    call r10\n");
            if (needs_pad) fprintf(cg->output, "    add rsp, 8\n");
        } else {
            /* Stage 68: N > 6 indirect call.
             * Callee addr was pushed first (deepest).  Push padding if
             * needed (accounting for the callee slot), push stack args
             * right-to-left, push reg args, pop reg args, then read
             * callee addr via rsp-relative load before calling. */
            int num_stack = nargs - 6;
            int pad = ((cg->push_depth + num_stack) % 2) != 0 ? 1 : 0;
            /* Note: push_depth already incremented by the callee push above.
             * We need (push_depth + pad + num_stack) % 2 == 0 for alignment.
             * Since push_depth was already incremented: pad compensates for
             * (updated_push_depth + num_stack) being odd. */
            if (pad) {
                fprintf(cg->output, "    sub rsp, 8\n");
                cg->push_depth++;
            }
            for (int i = nargs - 1; i >= 6; i--) {
                codegen_expression(cg, node->children[i + 1]);
                EMIT_INDIRECT_ARG_CONVERT(fn, node, i);
                fprintf(cg->output, "    push rax\n");
                cg->push_depth++;
            }
            for (int i = 0; i < 6; i++) {
                codegen_expression(cg, node->children[i + 1]);
                EMIT_INDIRECT_ARG_CONVERT(fn, node, i);
                fprintf(cg->output, "    push rax\n");
                cg->push_depth++;
            }
            for (int i = 5; i >= 0; i--) {
                fprintf(cg->output, "    pop %s\n", arg_regs[i]);
                cg->push_depth--;
            }
            /* Callee addr is below stack args and padding. */
            int callee_slot = (num_stack + pad) * 8;
            fprintf(cg->output, "    mov r10, [rsp + %d]\n", callee_slot);
            fprintf(cg->output, "    call r10\n");
            int cleanup = (num_stack + pad + 1) * 8;
            fprintf(cg->output, "    add rsp, %d\n", cleanup);
            cg->push_depth -= (num_stack + pad + 1);
        }
#undef EMIT_INDIRECT_ARG_CONVERT
        /* Result type from the function's declared return type. */
        Type *ret = fn->base;
        node->result_type = ret->kind;
        node->decl_type = ret->kind;
        if (ret->kind == TYPE_POINTER) {
            node->full_type = ret;
        }
        return;
    }
    if (node->type == AST_BINARY_OP) {
        const char *bop = node->value;
        /* Short-circuit logical operators — do not evaluate RHS unconditionally */
        if (strcmp(bop, "&&") == 0) {
            int label_id = cg->label_count++;
            codegen_expression(cg, node->children[0]);
            if (node->children[0]->result_type == TYPE_LONG) {
                fprintf(cg->output, "    cmp rax, 0\n");
            } else {
                fprintf(cg->output, "    cmp eax, 0\n");
            }
            fprintf(cg->output, "    je .L_and_false_%d\n", label_id);
            codegen_expression(cg, node->children[1]);
            if (node->children[1]->result_type == TYPE_LONG) {
                fprintf(cg->output, "    cmp rax, 0\n");
            } else {
                fprintf(cg->output, "    cmp eax, 0\n");
            }
            fprintf(cg->output, "    setne al\n");
            fprintf(cg->output, "    movzx eax, al\n");
            fprintf(cg->output, "    jmp .L_and_end_%d\n", label_id);
            fprintf(cg->output, ".L_and_false_%d:\n", label_id);
            fprintf(cg->output, "    mov eax, 0\n");
            fprintf(cg->output, ".L_and_end_%d:\n", label_id);
            node->result_type = TYPE_INT;
            return;
        }
        if (strcmp(bop, "||") == 0) {
            int label_id = cg->label_count++;
            codegen_expression(cg, node->children[0]);
            if (node->children[0]->result_type == TYPE_LONG) {
                fprintf(cg->output, "    cmp rax, 0\n");
            } else {
                fprintf(cg->output, "    cmp eax, 0\n");
            }
            fprintf(cg->output, "    jne .L_or_true_%d\n", label_id);
            codegen_expression(cg, node->children[1]);
            if (node->children[1]->result_type == TYPE_LONG) {
                fprintf(cg->output, "    cmp rax, 0\n");
            } else {
                fprintf(cg->output, "    cmp eax, 0\n");
            }
            fprintf(cg->output, "    setne al\n");
            fprintf(cg->output, "    movzx eax, al\n");
            fprintf(cg->output, "    jmp .L_or_end_%d\n", label_id);
            fprintf(cg->output, ".L_or_true_%d:\n", label_id);
            fprintf(cg->output, "    mov eax, 1\n");
            fprintf(cg->output, ".L_or_end_%d:\n", label_id);
            node->result_type = TYPE_INT;
            return;
        }
        /* Stage 16-03: shift operators `<<` and `>>` are integer-only
         * and the result type follows the promoted type of the LEFT
         * operand alone. The right operand is evaluated, integer-
         * promoted, and only its low byte (cl) is consumed as the
         * shift count. Pointer or array operands on either side are
         * rejected with the existing diagnostic shape. */
        /* Stage 16-04: bitwise binary operators `&` / `^` / `|` are
         * integer-only. Operands undergo the usual integer promotion
         * (char/short → int; long stays long; mixed → long). The
         * result type follows the common arithmetic type. Pointer or
         * array operands on either side are rejected with the
         * existing diagnostic shape. */
        if (strcmp(node->value, "&") == 0 || strcmp(node->value, "^") == 0 ||
            strcmp(node->value, "|") == 0) {
            const char *bw = node->value;
            TypeKind lt = expr_result_type(cg, node->children[0]);
            TypeKind rt = expr_result_type(cg, node->children[1]);
            if (lt == TYPE_POINTER || rt == TYPE_POINTER) {
                compile_error(
                        "error: operator '%s' not supported on pointer operands\n",
                        bw);
            }
            TypeKind common = common_arith_kind(lt, rt);
            codegen_expression(cg, node->children[0]);
            if (common == TYPE_LONG &&
                node->children[0]->result_type != TYPE_LONG) {
                fprintf(cg->output, "    movsxd rax, eax\n");
            }
            fprintf(cg->output, "    push rax\n");
            cg->push_depth++;
            codegen_expression(cg, node->children[1]);
            if (common == TYPE_LONG &&
                node->children[1]->result_type != TYPE_LONG) {
                fprintf(cg->output, "    movsxd rax, eax\n");
            }
            fprintf(cg->output, "    pop rcx\n");
            cg->push_depth--;
            const char *insn = (strcmp(bw, "&") == 0) ? "and"
                             : (strcmp(bw, "^") == 0) ? "xor" : "or";
            if (common == TYPE_LONG) {
                fprintf(cg->output, "    %s rax, rcx\n", insn);
            } else {
                fprintf(cg->output, "    %s eax, ecx\n", insn);
            }
            node->result_type = common;
            /* Stage 40: propagate unsigned result from UAC. */
            node->is_unsigned = uac_is_unsigned(
                node->children[0]->result_type, node->children[0]->is_unsigned,
                node->children[1]->result_type, node->children[1]->is_unsigned);
            return;
        }
        if (strcmp(node->value, "<<") == 0 || strcmp(node->value, ">>") == 0) {
            const char *sop = node->value;
            TypeKind lt = expr_result_type(cg, node->children[0]);
            TypeKind rt = expr_result_type(cg, node->children[1]);
            if (lt == TYPE_POINTER || rt == TYPE_POINTER) {
                compile_error(
                        "error: operator '%s' not supported on pointer operands\n",
                        sop);
            }
            codegen_expression(cg, node->children[0]);
            fprintf(cg->output, "    push rax\n");
            cg->push_depth++;
            codegen_expression(cg, node->children[1]);
            fprintf(cg->output, "    pop rcx\n");
            cg->push_depth--;
            /* After the pop, rcx = left, rax = right (count). Swap so
             * left is in rax and the count's low byte is addressable
             * as cl. */
            fprintf(cg->output, "    xchg rax, rcx\n");
            TypeKind result = promote_kind(node->children[0]->result_type);
            /* Stage 40: unsigned right shift uses shr; signed uses sar. */
            const char *insn;
            if (strcmp(sop, "<<") == 0) {
                insn = "shl";
            } else if (node->children[0]->is_unsigned) {
                insn = "shr";
            } else {
                insn = "sar";
            }
            if (result == TYPE_LONG) {
                fprintf(cg->output, "    %s rax, cl\n", insn);
            } else {
                fprintf(cg->output, "    %s eax, cl\n", insn);
            }
            node->result_type = result;
            node->is_unsigned = node->children[0]->is_unsigned;
            return;
        }
        const char *op = node->value;
        int is_arith = (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
                        strcmp(op, "*") == 0 || strcmp(op, "/") == 0 ||
                        strcmp(op, "%") == 0);
        int is_cmp = (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
                      strcmp(op, "<")  == 0 || strcmp(op, "<=") == 0 ||
                      strcmp(op, ">")  == 0 || strcmp(op, ">=") == 0);
        /* For arithmetic and comparison operators, select a common type
         * after promotion. If the common type is long, both operands must
         * live in the full rax before the op — sign-extend int-sized
         * sides with movsxd. */
        TypeKind common = TYPE_INT;
        /* Stage 12-06: a pointer operand on either side of `==` or
         * `!=` makes this a pointer comparison: use the 64-bit cmp
         * path and skip the integer movsxd widening on pointer
         * operands (they are already in the full rax).
         *
         * Stage 13-04: `T* + int`, `int + T*`, and `T* - int` are
         * pointer arithmetic — the integer side is scaled by
         * sizeof(*p) before the add/sub. `T* + T*`, `int - T*`,
         * and other operators with a pointer operand are rejected.
         */
        int is_pointer_cmp = 0;
        int is_pointer_arith = 0;
        int is_pointer_diff = 0;
        if (is_arith || is_cmp) {
            TypeKind lt = expr_result_type(cg, node->children[0]);
            TypeKind rt = expr_result_type(cg, node->children[1]);
            if (lt == TYPE_POINTER || rt == TYPE_POINTER) {
                if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
                    is_pointer_cmp = 1;
                    common = TYPE_LONG;
                } else if (strcmp(op, "+") == 0) {
                    if (lt == TYPE_POINTER && rt == TYPE_POINTER) {
                        compile_error(
                                "error: cannot add two pointers\n");
                    }
                    is_pointer_arith = 1;
                    common = TYPE_LONG;
                } else if (strcmp(op, "-") == 0) {
                    if (lt == TYPE_POINTER && rt == TYPE_POINTER) {
                        /* Stage 41: pointer difference p1 - p2. */
                        is_pointer_diff = 1;
                        common = TYPE_LONG;
                    } else {
                        if (rt == TYPE_POINTER) {
                            compile_error(
                                    "error: cannot subtract pointer from integer\n");
                        }
                        is_pointer_arith = 1;
                        common = TYPE_LONG;
                    }
                } else {
                    compile_error(
                            "error: operator '%s' not supported on pointer operands\n",
                            op);
                }
            } else {
                common = common_arith_kind(lt, rt);
            }
        }

        /* Evaluate left into rax/eax */
        codegen_expression(cg, node->children[0]);
        if ((is_arith || is_cmp) && common == TYPE_LONG &&
            node->children[0]->result_type != TYPE_LONG &&
            node->children[0]->result_type != TYPE_POINTER &&
            !node->children[0]->is_unsigned) {
            /* Stage 40: unsigned 32-bit is already zero-extended; skip movsxd. */
            fprintf(cg->output, "    movsxd rax, eax\n");
        }
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;
        /* Evaluate right into rax/eax */
        codegen_expression(cg, node->children[1]);
        if ((is_arith || is_cmp) && common == TYPE_LONG &&
            node->children[1]->result_type != TYPE_LONG &&
            node->children[1]->result_type != TYPE_POINTER &&
            !node->children[1]->is_unsigned) {
            fprintf(cg->output, "    movsxd rax, eax\n");
        }
        /* Pop left into rcx; now rcx=left, rax=right */
        fprintf(cg->output, "    pop rcx\n");
        cg->push_depth--;

        /* Stage 12-06: validate pointer comparison operand combinations
         * after both sides are evaluated (so result_type / full_type
         * are populated). Two pointers: chains must match. Pointer +
         * integer: the integer side must be the null pointer constant
         * `0`; any non-zero integer is rejected. */
        if (is_pointer_cmp) {
            ASTNode *lhs = node->children[0];
            ASTNode *rhs = node->children[1];
            int lhs_ptr = (lhs->result_type == TYPE_POINTER);
            int rhs_ptr = (rhs->result_type == TYPE_POINTER);
            if (lhs_ptr && rhs_ptr) {
                if (!pointer_types_equal(lhs->full_type, rhs->full_type)) {
                    compile_error(
                            "error: incompatible pointer types in comparison\n");
                }
            } else if (lhs_ptr && !rhs_ptr) {
                if (!is_null_pointer_constant(rhs)) {
                    compile_error(
                            "error: comparing pointer with non zero integer\n");
                }
            } else if (!lhs_ptr && rhs_ptr) {
                if (!is_null_pointer_constant(lhs)) {
                    compile_error(
                            "error: comparing pointer with non zero integer\n");
                }
            }
        }

        /* Stage 13-04: pointer arithmetic. By the time we get here both
         * sides are evaluated; rcx = LHS, rax = RHS. The existing
         * widening at LHS/RHS evaluation has already promoted the
         * integer side to 64 bits (common == TYPE_LONG). The pointer
         * side is in full rax/rcx unchanged. Scale the integer side
         * by sizeof(*p), then add/subtract. The pointer side has
         * already populated full_type via codegen_expression. */
        if (is_pointer_arith) {
            int lhs_is_ptr = (node->children[0]->result_type == TYPE_POINTER);
            Type *ptr_type = lhs_is_ptr ? node->children[0]->full_type
                                        : node->children[1]->full_type;
            if (!ptr_type || ptr_type->kind != TYPE_POINTER) {
                compile_error(
                        "error: pointer arithmetic missing pointer type\n");
            }
            /* Stage 38: pointer arithmetic on void * is not allowed. */
            if (ptr_type->base && ptr_type->base->kind == TYPE_VOID) {
                compile_error(
                        "error: cannot perform pointer arithmetic on void pointer\n");
            }
            /* Stage 41: pointer arithmetic on function pointers is not allowed. */
            if (ptr_type->base && ptr_type->base->kind == TYPE_FUNCTION) {
                compile_error(
                        "error: cannot perform pointer arithmetic on function pointer\n");
            }
            int elem_size = type_size(ptr_type->base);
            if (lhs_is_ptr) {
                /* Pointer in rcx, integer in rax. */
                if (elem_size != 1) {
                    fprintf(cg->output, "    imul rax, rax, %d\n", elem_size);
                }
                if (strcmp(op, "+") == 0) {
                    fprintf(cg->output, "    add rax, rcx\n");
                } else { /* "-" */
                    fprintf(cg->output, "    sub rcx, rax\n");
                    fprintf(cg->output, "    mov rax, rcx\n");
                }
            } else {
                /* Integer in rcx, pointer in rax. Op is "+" — `int - ptr`
                 * was rejected in the type-check phase above. */
                if (elem_size != 1) {
                    fprintf(cg->output, "    imul rcx, rcx, %d\n", elem_size);
                }
                fprintf(cg->output, "    add rax, rcx\n");
            }
            node->result_type = TYPE_POINTER;
            node->full_type = ptr_type;
            return;
        }

        /* Stage 41: pointer difference p1 - p2. Both sides evaluated;
         * rcx = p1 (LHS), rax = p2 (RHS). Validate matching pointee
         * types, subtract addresses (byte delta), divide by sizeof(*p1)
         * to produce a signed element-count (ptrdiff_t-equivalent long). */
        if (is_pointer_diff) {
            Type *lptr = node->children[0]->full_type;
            Type *rptr = node->children[1]->full_type;
            if (!lptr || lptr->kind != TYPE_POINTER ||
                !rptr || rptr->kind != TYPE_POINTER) {
                compile_error(
                        "error: pointer subtraction missing pointer type\n");
            }
            if ((lptr->base && lptr->base->kind == TYPE_VOID) ||
                (rptr->base && rptr->base->kind == TYPE_VOID)) {
                compile_error(
                        "error: cannot perform pointer arithmetic on void pointer\n");
            }
            if ((lptr->base && lptr->base->kind == TYPE_FUNCTION) ||
                (rptr->base && rptr->base->kind == TYPE_FUNCTION)) {
                compile_error(
                        "error: cannot perform pointer arithmetic on function pointer\n");
            }
            if (!pointer_types_equal(lptr, rptr)) {
                compile_error(
                        "error: incompatible pointer types in subtraction\n");
            }
            int elem_size = type_size(lptr->base);
            /* rcx = p1, rax = p2; compute p1 - p2 in bytes then divide. */
            fprintf(cg->output, "    sub rcx, rax\n");
            fprintf(cg->output, "    mov rax, rcx\n");
            if (elem_size > 1) {
                fprintf(cg->output, "    cqo\n");
                fprintf(cg->output, "    mov rcx, %d\n", elem_size);
                fprintf(cg->output, "    idiv rcx\n");
            }
            node->result_type = TYPE_LONG;
            return;
        }

        /* Stage 40: compute result signedness via UAC now that both children
         * have been evaluated and their is_unsigned flags are set. */
        int op_is_unsigned = 0;
        if (!is_pointer_cmp && !is_pointer_arith && !is_pointer_diff && (is_arith || is_cmp)) {
            op_is_unsigned = uac_is_unsigned(
                node->children[0]->result_type, node->children[0]->is_unsigned,
                node->children[1]->result_type, node->children[1]->is_unsigned);
        }

        if (is_arith && common == TYPE_LONG) {
            if (strcmp(op, "+") == 0) {
                fprintf(cg->output, "    add rax, rcx\n");
            } else if (strcmp(op, "-") == 0) {
                /* left - right: rcx - rax */
                fprintf(cg->output, "    sub rcx, rax\n");
                fprintf(cg->output, "    mov rax, rcx\n");
            } else if (strcmp(op, "*") == 0) {
                fprintf(cg->output, "    imul rax, rcx\n");
            } else if (strcmp(op, "/") == 0) {
                /* left / right: rcx / rax */
                fprintf(cg->output, "    xchg rax, rcx\n");
                if (op_is_unsigned) {
                    fprintf(cg->output, "    xor edx, edx\n");
                    fprintf(cg->output, "    div rcx\n");
                } else {
                    fprintf(cg->output, "    cqo\n");
                    fprintf(cg->output, "    idiv rcx\n");
                }
            } else { /* "%" */
                /* left % right: remainder of rcx / rax. */
                fprintf(cg->output, "    xchg rax, rcx\n");
                if (op_is_unsigned) {
                    fprintf(cg->output, "    xor edx, edx\n");
                    fprintf(cg->output, "    div rcx\n");
                } else {
                    fprintf(cg->output, "    cqo\n");
                    fprintf(cg->output, "    idiv rcx\n");
                }
                fprintf(cg->output, "    mov rax, rdx\n");
            }
            node->result_type = TYPE_LONG;
            node->is_unsigned = op_is_unsigned;
            return;
        }
        if (strcmp(op, "+") == 0) {
            fprintf(cg->output, "    add eax, ecx\n");
            node->result_type = TYPE_INT;
            node->is_unsigned = op_is_unsigned;
        } else if (strcmp(op, "-") == 0) {
            /* left - right: ecx - eax */
            fprintf(cg->output, "    sub ecx, eax\n");
            fprintf(cg->output, "    mov eax, ecx\n");
            node->result_type = TYPE_INT;
            node->is_unsigned = op_is_unsigned;
        } else if (strcmp(op, "*") == 0) {
            fprintf(cg->output, "    imul eax, ecx\n");
            node->result_type = TYPE_INT;
            node->is_unsigned = op_is_unsigned;
        } else if (strcmp(op, "/") == 0) {
            /* left / right: ecx / eax */
            fprintf(cg->output, "    xchg eax, ecx\n");
            if (op_is_unsigned) {
                fprintf(cg->output, "    xor edx, edx\n");
                fprintf(cg->output, "    div ecx\n");
            } else {
                fprintf(cg->output, "    cdq\n");
                fprintf(cg->output, "    idiv ecx\n");
            }
            node->result_type = TYPE_INT;
            node->is_unsigned = op_is_unsigned;
        } else if (strcmp(op, "%") == 0) {
            /* left % right: remainder of ecx / eax. */
            fprintf(cg->output, "    xchg eax, ecx\n");
            if (op_is_unsigned) {
                fprintf(cg->output, "    xor edx, edx\n");
                fprintf(cg->output, "    div ecx\n");
            } else {
                fprintf(cg->output, "    cdq\n");
                fprintf(cg->output, "    idiv ecx\n");
            }
            fprintf(cg->output, "    mov eax, edx\n");
            node->result_type = TYPE_INT;
            node->is_unsigned = op_is_unsigned;
        } else {
            /* Comparisons: compare left (rcx/ecx) with right (rax/eax),
             * using the width of the common type after promotion. Result
             * is a normalized 0/1 in eax of type int.
             * Stage 40: use unsigned setcc variants when op_is_unsigned. */
            const char *setcc = NULL;
            if      (strcmp(op, "==") == 0) setcc = "sete";
            else if (strcmp(op, "!=") == 0) setcc = "setne";
            else if (op_is_unsigned) {
                if      (strcmp(op, "<")  == 0) setcc = "setb";
                else if (strcmp(op, "<=") == 0) setcc = "setbe";
                else if (strcmp(op, ">")  == 0) setcc = "seta";
                else if (strcmp(op, ">=") == 0) setcc = "setae";
            } else {
                if      (strcmp(op, "<")  == 0) setcc = "setl";
                else if (strcmp(op, "<=") == 0) setcc = "setle";
                else if (strcmp(op, ">")  == 0) setcc = "setg";
                else if (strcmp(op, ">=") == 0) setcc = "setge";
            }
            if (common == TYPE_LONG) {
                fprintf(cg->output, "    cmp rcx, rax\n");
            } else {
                fprintf(cg->output, "    cmp ecx, eax\n");
            }
            fprintf(cg->output, "    %s al\n", setcc);
            fprintf(cg->output, "    movzx eax, al\n");
            node->result_type = TYPE_INT;
        }
        return;
    }
    if (node->type == AST_CONDITIONAL_EXPR) {
        int label_id = cg->label_count++;
        ASTNode *cond_node  = node->children[0];
        ASTNode *true_node  = node->children[1];
        ASTNode *false_node = node->children[2];

        codegen_expression(cg, cond_node);
        if (cond_node->result_type == TYPE_LONG || cond_node->result_type == TYPE_POINTER) {
            fprintf(cg->output, "    cmp rax, 0\n");
        } else {
            fprintf(cg->output, "    cmp eax, 0\n");
        }
        fprintf(cg->output, "    je .L_cond_false_%d\n", label_id);

        codegen_expression(cg, true_node);
        fprintf(cg->output, "    jmp .L_cond_end_%d\n", label_id);

        fprintf(cg->output, ".L_cond_false_%d:\n", label_id);
        codegen_expression(cg, false_node);
        fprintf(cg->output, ".L_cond_end_%d:\n", label_id);

        TypeKind tk = true_node->result_type;
        TypeKind fk = false_node->result_type;
        int true_is_null  = is_null_pointer_constant(true_node);
        int false_is_null = is_null_pointer_constant(false_node);

        if (tk == TYPE_POINTER && fk == TYPE_POINTER) {
            if (!pointer_types_equal(true_node->full_type, false_node->full_type)) {
                compile_error(
                        "error: conditional operator has incompatible pointer types\n");
            }
            node->result_type = TYPE_POINTER;
            node->full_type   = true_node->full_type;
        } else if (tk == TYPE_POINTER && false_is_null) {
            node->result_type = TYPE_POINTER;
            node->full_type   = true_node->full_type;
        } else if (fk == TYPE_POINTER && true_is_null) {
            node->result_type = TYPE_POINTER;
            node->full_type   = false_node->full_type;
        } else {
            node->result_type = common_arith_kind(promote_kind(tk), promote_kind(fk));
        }
        return;
    }
    if (node->type == AST_COMMA_EXPR) {
        codegen_expression(cg, node->children[0]);
        codegen_expression(cg, node->children[1]);
        node->result_type = node->children[1]->result_type;
        node->full_type   = node->children[1]->full_type;
        return;
    }
    /* Stage 75-04: __builtin_va_start initializes the va_list fields.
     * __builtin_va_end is a no-op per spec.  va_copy and va_arg remain
     * stubs (out of scope for this stage). */
    if (node->type == AST_BUILTIN_VA_START) {
        /* children[0] = ap (va_list local), children[1] = last named param */
        ASTNode *ap_node = node->children[0];
        LocalVar *lv_ap = codegen_find_var(cg, ap_node->value);
        if (!lv_ap) {
            compile_error("error: va_start: 'ap' is not a local variable\n");
        }
        int ap_off = lv_ap->offset;
        /* gp_offset = named_gp_params * 8, already clamped to ≤ 48 */
        int gp_off_val  = cg->variadic_named_gp_params * 8;
        /* fp_offset = 48 (start of reserved FP/XMM area) */
        int fp_off_val  = 48;
        /* overflow_arg_area = rbp + 16 + 8 * named_stack_params */
        int overflow_disp = 16 + cg->variadic_named_stack_params * 8;
        /* ap[0].gp_offset (unsigned int, 4 bytes at ap+0) */
        fprintf(cg->output, "    mov dword [rbp - %d], %d\n", ap_off, gp_off_val);
        /* ap[0].fp_offset (unsigned int, 4 bytes at ap+4) */
        fprintf(cg->output, "    mov dword [rbp - %d], %d\n", ap_off - 4, fp_off_val);
        /* ap[0].overflow_arg_area (void *, 8 bytes at ap+8) */
        fprintf(cg->output, "    lea rax, [rbp + %d]\n", overflow_disp);
        fprintf(cg->output, "    mov [rbp - %d], rax\n", ap_off - 8);
        /* ap[0].reg_save_area (void *, 8 bytes at ap+16) */
        fprintf(cg->output, "    lea rax, [rbp - %d]\n", cg->variadic_reg_save_offset);
        fprintf(cg->output, "    mov [rbp - %d], rax\n", ap_off - 16);
        node->result_type = TYPE_VOID;
        return;
    }
    if (node->type == AST_BUILTIN_VA_END ||
        node->type == AST_BUILTIN_VA_COPY) {
        node->result_type = TYPE_VOID;
        return;
    }
    if (node->type == AST_BUILTIN_VA_ARG) {
        /* Stage 75-06: va_arg codegen for GP register class types.
         * Implements the SysV AMD64 ABI va_arg algorithm for integer
         * and pointer types using the existing va_list layout:
         *   [ap+0]  gp_offset          (unsigned int, 4 bytes)
         *   [ap+8]  overflow_arg_area  (void *, 8 bytes)
         *   [ap+16] reg_save_area      (void *, 8 bytes) */
        Type *arg_type = node->full_type;
        TypeKind kind = arg_type ? arg_type->kind : (TypeKind)node->decl_type;

        /* Reject unsupported types */
        switch (kind) {
        case TYPE_VOID:
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_SHORT:
            compile_error("error: va_arg: type %s is not supported;"
                          " use int or a larger type\n",
                          type_kind_name(kind));
            return;
        case TYPE_STRUCT:
        case TYPE_UNION:
            compile_error("error: va_arg: aggregate types are not supported\n");
            return;
        case TYPE_ARRAY:
            compile_error("error: va_arg: array types are not supported\n");
            return;
        default:
            break;
        }

        ASTNode *ap_node = node->children[0];
        LocalVar *lv_ap = codegen_find_var(cg, ap_node->value);
        if (!lv_ap) {
            compile_error("error: va_arg: 'ap' is not a local variable\n");
        }
        int ap_off = lv_ap->offset;
        int lbl = cg->label_count++;

        /* Load gp_offset; branch to overflow if >= 48 */
        fprintf(cg->output, "    mov eax, dword [rbp - %d]\n", ap_off);
        fprintf(cg->output, "    cmp eax, 48\n");
        fprintf(cg->output, "    jae .L_va_arg_ovf_%d\n", lbl);

        /* GP register save area path: src = reg_save_area + gp_offset */
        fprintf(cg->output, "    mov rcx, [rbp - %d]\n", ap_off - 16);
        fprintf(cg->output, "    lea rdx, [rcx + rax]\n");
        fprintf(cg->output, "    add eax, 8\n");
        fprintf(cg->output, "    mov dword [rbp - %d], eax\n", ap_off);
        fprintf(cg->output, "    jmp .L_va_arg_load_%d\n", lbl);

        /* Overflow stack area path: src = overflow_arg_area */
        fprintf(cg->output, ".L_va_arg_ovf_%d:\n", lbl);
        fprintf(cg->output, "    mov rdx, [rbp - %d]\n", ap_off - 8);
        fprintf(cg->output, "    lea rcx, [rdx + 8]\n");
        fprintf(cg->output, "    mov [rbp - %d], rcx\n", ap_off - 8);

        /* Load the value from [rdx] according to the requested type */
        fprintf(cg->output, ".L_va_arg_load_%d:\n", lbl);
        switch (kind) {
        case TYPE_INT:
            /* 4-byte GP slot: load 4 bytes; zero-extends to rax automatically */
            fprintf(cg->output, "    mov eax, dword [rdx]\n");
            node->result_type = TYPE_INT;
            node->is_unsigned = (arg_type && !arg_type->is_signed) ? 1 : 0;
            break;
        case TYPE_LONG:
        case TYPE_LONG_LONG:
        case TYPE_UNSIGNED_LONG_LONG:
        case TYPE_POINTER:
            /* 8-byte GP slot */
            fprintf(cg->output, "    mov rax, [rdx]\n");
            node->result_type = kind;
            node->is_unsigned = (arg_type && !arg_type->is_signed) ? 1 : 0;
            break;
        default:
            compile_error("error: va_arg: type not in GP register class\n");
            return;
        }
        node->full_type = arg_type;
        return;
    }
}

/*
 * Emit a zero-compare on the result register using the width of
 * `cond`'s result type, so long-typed conditions test all 64 bits
 * instead of just the low 32.
 */
static void emit_cond_cmp_zero(CodeGen *cg, ASTNode *cond) {
    if (cond && cond->result_type == TYPE_LONG) {
        fprintf(cg->output, "    cmp rax, 0\n");
    } else {
        fprintf(cg->output, "    cmp eax, 0\n");
    }
}

/*
 * Stage 44: recursively initialize a local struct slot from a brace-list.
 * base_offset is the rbp-relative address of the first byte of the struct.
 * list is an AST_INITIALIZER_LIST (may have fewer elements than st->field_count).
 * The caller must have already zero-filled the memory region.
 */
static void emit_local_struct_init(CodeGen *cg, Type *st, int base_offset,
                                   ASTNode *list) {
    int ninit = list ? list->child_count : 0;
    if (ninit > st->field_count) {
        compile_error( "error: too many initializers for struct\n");
    }
    for (int i = 0; i < ninit; i++) {
        StructField *f = &st->fields[i];
        int foffset = base_offset - f->offset;
        int fsize   = f->full_type ? f->full_type->size : type_kind_bytes(f->kind);
        ASTNode *elem = list->children[i];

        if (f->kind == TYPE_STRUCT && f->full_type &&
            elem->type == AST_INITIALIZER_LIST) {
            /* Nested struct field — recurse. */
            emit_local_struct_init(cg, f->full_type, foffset, elem);
        } else if (f->kind == TYPE_POINTER) {
            /* Pointer field: initializer must produce a pointer value. */
            codegen_expression(cg, elem);
            if (elem->result_type != TYPE_POINTER &&
                !is_null_pointer_constant(elem)) {
                compile_error(
                        "error: incompatible field initializer for pointer field '%s'\n",
                        f->name);
            }
            emit_store_local(cg, foffset, fsize, 1);
        } else {
            /* Scalar field: string literals are not valid for non-pointer fields. */
            if (elem->type == AST_STRING_LITERAL) {
                compile_error(
                        "error: incompatible field initializer for field '%s'\n",
                        f->name);
            }
            codegen_expression(cg, elem);
            int src_is_long = (elem->result_type == TYPE_LONG ||
                               elem->result_type == TYPE_POINTER);
            emit_store_local(cg, foffset, fsize, src_is_long);
        }
    }
}

static void codegen_statement(CodeGen *cg, ASTNode *node, int is_main) {
    if (node->type == AST_DECLARATION) {
        /* Duplicate check limited to the current scope only — shadowing is allowed. */
        for (int i = cg->scope_start; i < cg->local_count; i++) {
            if (strcmp(cg->locals[i].name, node->value) == 0) {
                compile_error( "error: duplicate declaration of variable '%s'\n", node->value);
            }
        }
        /* Stage 71: block-scope static variable. */
        if (node->storage_class == SC_STATIC) {
            if (node->decl_type == TYPE_ARRAY || node->decl_type == TYPE_STRUCT ||
                node->decl_type == TYPE_UNION) {
                compile_error(
                        "error: static local arrays, structs and unions are not yet supported\n");
            }
            if (cg->local_static_count >= MAX_LOCAL_STATICS) {
                compile_error(
                        "error: too many local static variables (max %d)\n",
                        MAX_LOCAL_STATICS);
            }
            /* Validate that the initializer (if any) is a compile-time constant. */
            long init_value = 0;
            int is_initialized = 0;
            if (node->child_count > 0) {
                ASTNode *init = node->children[0];
                if (init->type == AST_INT_LITERAL) {
                    init_value = strtol(init->value, NULL, 10);
                    is_initialized = 1;
                } else if (init->type == AST_CHAR_LITERAL) {
                    init_value = (long)(unsigned char)init->value[0];
                    is_initialized = 1;
                } else if (init->type == AST_UNARY_OP &&
                           strcmp(init->value, "-") == 0 &&
                           init->child_count > 0 &&
                           init->children[0]->type == AST_INT_LITERAL) {
                    init_value = -strtol(init->children[0]->value, NULL, 10);
                    is_initialized = 1;
                } else {
                    compile_error(
                            "error: initializer for block-scope static '%s' must be a constant expression\n",
                            node->value);
                }
            }
            /* Generate a unique label: Lstatic_<func>_<counter>. */
            char label[256];
            snprintf(label, sizeof(label), "Lstatic_%s_%d",
                     cg->current_func, cg->label_count++);
            /* Register in the local variable table so scope and shadowing work.
             * Don't advance stack_offset — statics are not stack-allocated. */
            if (cg->local_count >= MAX_LOCALS) {
                compile_error("error: too many local variables\n");
            }
            LocalVar *lv = &cg->locals[cg->local_count];
            strncpy(lv->name, node->value, 255);
            lv->name[255] = '\0';
            lv->offset = 0;
            lv->size = type_kind_bytes(node->decl_type);
            lv->kind = node->decl_type;
            lv->full_type = node->full_type;
            lv->is_const = node->is_const;
            lv->is_unsigned = node->is_unsigned;
            lv->is_static = 1;
            strncpy(lv->static_label, label, 255);
            lv->static_label[255] = '\0';
            cg->local_count++;
            /* Add to the deferred emission pool (.data or .bss). */
            LocalStaticVar *sv = &cg->local_statics[cg->local_static_count++];
            strncpy(sv->label, label, 255);
            sv->label[255] = '\0';
            sv->kind = node->decl_type;
            sv->full_type = node->full_type;
            sv->size = type_kind_bytes(node->decl_type);
            sv->is_initialized = is_initialized;
            sv->init_value = init_value;
            sv->is_unsigned = node->is_unsigned;
            return;
        }
        /* Stage 13-01: array locals get sized from the array Type
         * (element_size * length) and aligned to the element's
         * natural alignment.
         *
         * Stage 14-06: a `char` array may carry an AST_STRING_LITERAL
         * child as its initializer. The literal's payload bytes are
         * copied into the slot in order, followed by an explicit NUL
         * terminator and zero fill out to the array's declared size.
         * The literal is not added to the .rodata pool — codegen
         * emits per-byte stack stores instead. */
        if ((node->decl_type == TYPE_STRUCT || node->decl_type == TYPE_UNION) &&
            node->full_type) {
            /* Stage 30/32: struct local. Stage 32 adds brace-initializer support.
             * Stage 72: union local — same machinery, first-member init via brace list. */
            if (node->full_type->size == 0) {
                compile_error(
                        "error: variable '%s' has incomplete %s type\n",
                        node->value, type_kind_name(node->decl_type));
            }
            int size = node->full_type->size;
            int align = node->full_type->alignment;
            int offset = codegen_add_var(cg, node->value, size, align,
                                         node->decl_type, node->full_type);
            cg->locals[cg->local_count - 1].is_const = node->is_const;
            if (node->child_count > 0 &&
                node->children[0]->type == AST_INITIALIZER_LIST) {
                /* Zero-fill the entire slot first, then store provided values.
                 * For unions, only the first member is initialized. */
                for (int b = 0; b < size; b++) {
                    fprintf(cg->output, "    mov byte [rbp - %d], 0\n", offset - b);
                }
                if (node->decl_type == TYPE_UNION) {
                    /* First-member initialization: initialize only the first field. */
                    ASTNode *list = node->children[0];
                    if (list->child_count > 1) {
                        compile_error(
                                "error: too many initializers for union '%s'\n",
                                node->value);
                    }
                    if (list->child_count == 1 && node->full_type->field_count > 0) {
                        StructField *f = &node->full_type->fields[0];
                        int fsize = f->full_type ? f->full_type->size
                                                 : type_kind_bytes(f->kind);
                        ASTNode *elem = list->children[0];
                        codegen_expression(cg, elem);
                        int src_is_long = (elem->result_type == TYPE_LONG ||
                                           elem->result_type == TYPE_POINTER);
                        emit_store_local(cg, offset, fsize, src_is_long);
                    }
                } else {
                    emit_local_struct_init(cg, node->full_type, offset,
                                           node->children[0]);
                }
            } else if (node->child_count > 0) {
                /* struct/union T d = c — copy from another variable of the same type. */
                ASTNode *init = node->children[0];
                if (init->type != AST_VAR_REF) {
                    compile_error( "error: %s initializer must be a %s variable\n",
                            type_kind_name(node->decl_type),
                            type_kind_name(node->decl_type));
                }
                LocalVar *src = codegen_find_var(cg, init->value);
                if (!src || src->kind != node->decl_type || !src->full_type) {
                    compile_error( "error: %s initializer must be a %s variable\n",
                            type_kind_name(node->decl_type),
                            type_kind_name(node->decl_type));
                }
                if (src->full_type != node->full_type) {
                    compile_error( "error: incompatible %s types in initializer for '%s'\n",
                            type_kind_name(node->decl_type), node->value);
                }
                for (int b = 0; b < size; b++) {
                    fprintf(cg->output, "    movzx eax, byte [rbp - %d]\n", src->offset - b);
                    fprintf(cg->output, "    mov [rbp - %d], al\n", offset - b);
                }
            }
            return;
        }
        if (node->decl_type == TYPE_ARRAY) {
            int size = node->full_type->size;
            int elem_size = node->full_type->base->size;
            int align = node->full_type->base->alignment;
            int length = node->full_type->length;
            int offset = codegen_add_var(cg, node->value, size, align,
                                         node->decl_type, node->full_type);
            cg->locals[cg->local_count - 1].is_const = node->is_const;
            if (node->child_count > 0 &&
                node->children[0]->type == AST_INITIALIZER_LIST) {
                /* Stage 32: brace-initializer list. Evaluate each element and
                 * store it; zero-fill any elements beyond the list length.
                 * Stage 44: struct elements handled via emit_local_struct_init. */
                ASTNode *list = node->children[0];
                if (list->child_count > length) {
                    compile_error(
                            "error: too many initializers for array '%s'\n",
                            node->value);
                }
                Type *elem_type = node->full_type->base;
                for (int i = 0; i < length; i++) {
                    int elem_offset = offset - i * elem_size;
                    if (i < list->child_count) {
                        ASTNode *elem = list->children[i];
                        if (elem_type && elem_type->kind == TYPE_STRUCT &&
                            elem->type == AST_INITIALIZER_LIST) {
                            /* Stage 44: struct element — zero-fill then recurse. */
                            for (int b = 0; b < elem_size; b++)
                                fprintf(cg->output, "    mov byte [rbp - %d], 0\n",
                                        elem_offset - b);
                            emit_local_struct_init(cg, elem_type, elem_offset, elem);
                        } else {
                            codegen_expression(cg, elem);
                            int src_is_long = (elem->result_type == TYPE_LONG ||
                                               elem->result_type == TYPE_POINTER);
                            emit_store_local(cg, elem_offset, elem_size, src_is_long);
                        }
                    } else {
                        /* zero-fill */
                        switch (elem_size) {
                        case 1: fprintf(cg->output, "    mov byte [rbp - %d], 0\n", elem_offset); break;
                        case 2: fprintf(cg->output, "    mov word [rbp - %d], 0\n", elem_offset); break;
                        case 8: fprintf(cg->output, "    mov qword [rbp - %d], 0\n", elem_offset); break;
                        default: fprintf(cg->output, "    mov dword [rbp - %d], 0\n", elem_offset); break;
                        }
                    }
                }
            } else if (node->child_count > 0) {
                /* String-literal initializer path (existing). */
                ASTNode *str = node->children[0];
                for (int i = 0; i < size; i++) {
                    unsigned char b = (i < str->byte_length)
                        ? (unsigned char)str->value[i]
                        : 0;
                    fprintf(cg->output,
                            "    mov byte [rbp - %d], %d\n",
                            offset - i, b);
                }
            }
            return;
        }
        int size = type_kind_bytes(node->decl_type);
        int offset = codegen_add_var(cg, node->value, size, size,
                                     node->decl_type, node->full_type);
        cg->locals[cg->local_count - 1].is_const = node->is_const;
        cg->locals[cg->local_count - 1].is_unsigned = node->is_unsigned;
        if (node->child_count > 0) {
            codegen_expression(cg, node->children[0]);
            TypeKind init_kind = node->children[0]->result_type;
            /* Stage 12-06: the integer literal `0` is a null pointer
             * constant when the LHS is a pointer; accept it without
             * the integer-vs-pointer rejection. `mov eax, 0` already
             * zero-extends rax, so the 8-byte store path stores the
             * correct null address. */
            int rhs_is_null_ptr = (node->decl_type == TYPE_POINTER &&
                                   is_null_pointer_constant(node->children[0]));
            /* Stage 12-05: pointer/non-pointer mismatches in an init
             * expression are rejected here. When both sides are
             * pointers, the chains must agree exactly. */
            if (!rhs_is_null_ptr &&
                (node->decl_type == TYPE_POINTER || init_kind == TYPE_POINTER)) {
                if (node->decl_type != TYPE_POINTER) {
                    compile_error(
                            "error: variable '%s' assigning pointer to non pointer\n",
                            node->value);
                }
                if (init_kind != TYPE_POINTER) {
                    compile_error(
                            "error: variable '%s' assigning non pointer to pointer\n",
                            node->value);
                }
                if (!pointer_types_assignable(node->full_type,
                                              node->children[0]->full_type)) {
                    compile_error(
                            "error: variable '%s' incompatible pointer type in initializer\n",
                            node->value);
                }
                /* Stage 66: warn when initializer discards const qualifier. */
                if (node->full_type && node->full_type->base &&
                    node->children[0]->full_type && node->children[0]->full_type->base &&
                    node->children[0]->full_type->base->is_const &&
                    !node->full_type->base->is_const) {
                    codegen_warn_const_discard(cg, "initialization of", node->value);
                }
            }
            /* Pointer values live in the full rax already (lea / 8-byte
             * load), so they take the same store path as long values
             * without the movsxd widening step. A null pointer constant
             * also takes the 8-byte path so the full slot is written.
             * Stage 40: unsigned 32-bit values are already zero-extended
             * in rax by x86-64 semantics, so also skip movsxd. */
            int rhs_is_long = (init_kind == TYPE_LONG ||
                               init_kind == TYPE_POINTER ||
                               rhs_is_null_ptr ||
                               (size == 8 && node->children[0]->is_unsigned));
            /* Stage 63: _Bool initializer normalizes any nonzero value to 1. */
            if (node->decl_type == TYPE_BOOL)
                emit_bool_normalize(cg, rhs_is_long);
            emit_store_local(cg, offset, size, rhs_is_long);
        }
    } else if (node->type == AST_DECL_LIST) {
        for (int i = 0; i < node->child_count; i++) {
            codegen_statement(cg, node->children[i], is_main);
        }
    } else if (node->type == AST_RETURN_STATEMENT) {
        if (node->child_count == 0) {
            /* bare return; — only valid in void functions */
            if (cg->current_return_type != TYPE_VOID) {
                compile_error(
                        "error: empty return statement in non-void function '%s'\n",
                        cg->current_func);
            }
            if (cg->has_frame) {
                fprintf(cg->output, "    mov rsp, rbp\n");
                fprintf(cg->output, "    pop rbp\n");
            }
            fprintf(cg->output, "    ret\n");
        } else {
            /* return with expression */
            if (cg->current_return_type == TYPE_VOID) {
                compile_error(
                        "error: void function '%s' cannot return a value\n",
                        cg->current_func);
            }
            /* Reject using a void function call result as a return value. */
            if (node->children[0]->type == AST_FUNCTION_CALL &&
                node->children[0]->decl_type == TYPE_VOID) {
                compile_error(
                        "error: cannot use void function result as return value\n");
            }
            /* Stage 12-06: a return of the literal `0` from a pointer
             * function is a null pointer constant; accept it before the
             * integer-vs-pointer match enforced below. `mov eax, 0`
             * already zero-extends to rax, so the value in the return
             * register is the null address. */
            int ret_is_null_ptr = (cg->current_return_type == TYPE_POINTER &&
                                   is_null_pointer_constant(node->children[0]));
            codegen_expression(cg, node->children[0]);
            TypeKind src_kind = node->children[0]->result_type;
            TypeKind dst_kind = cg->current_return_type;
            /* Stage 12-05: when either side is a pointer, no integer
             * conversion applies — enforce strict type matching instead.
             * The pointer value is already in the full rax. */
            if (ret_is_null_ptr) {
                /* null pointer constant: no conversion needed */
            } else if (dst_kind == TYPE_POINTER || src_kind == TYPE_POINTER) {
                if (dst_kind != TYPE_POINTER) {
                    compile_error(
                            "error: function '%s' returning pointer from non pointer function\n",
                            cg->current_func);
                }
                if (src_kind != TYPE_POINTER) {
                    compile_error(
                            "error: function '%s' returning non pointer; expected pointer\n",
                            cg->current_func);
                }
                /* Stage 38: void* return is compatible with any object pointer return type,
                 * and any object pointer is compatible with a void* return type. */
                if (!pointer_types_assignable(node->children[0]->full_type,
                                              cg->current_return_full_type) &&
                    !pointer_types_assignable(cg->current_return_full_type,
                                              node->children[0]->full_type)) {
                    if (!pointer_types_equal(node->children[0]->full_type,
                                             cg->current_return_full_type)) {
                        compile_error(
                                "error: function '%s' returning incorrect pointer type\n",
                                cg->current_func);
                    }
                }
            } else {
                /* Convert the result to the function's declared return type
                 * before placing it in the return register. Narrowing to int
                 * is implicit (eax already holds the low 32 bits of rax); all
                 * other size changes emit an explicit sign-extend. */
                emit_convert(cg, src_kind, dst_kind);
            }
            /* Stage 14-07: main now returns normally so the gcc-linked
             * crt0 / __libc_start_main can call libc `exit`, which runs
             * stdio cleanup (notably flushing buffered `puts` output to
             * non-TTY stdouts). The exit code in eax becomes
             * __libc_start_main's return-from-main value, which it then
             * passes to exit. */
            if (cg->has_frame) {
                fprintf(cg->output, "    mov rsp, rbp\n");
                fprintf(cg->output, "    pop rbp\n");
            }
            fprintf(cg->output, "    ret\n");
        }
    } else if (node->type == AST_IF_STATEMENT) {
        int label_id = cg->label_count++;
        codegen_expression(cg, node->children[0]);
        emit_cond_cmp_zero(cg, node->children[0]);
        if (node->child_count == 3) {
            /* if/else */
            fprintf(cg->output, "    je .L_else_%d\n", label_id);
            codegen_statement(cg, node->children[1], is_main);
            fprintf(cg->output, "    jmp .L_end_%d\n", label_id);
            fprintf(cg->output, ".L_else_%d:\n", label_id);
            codegen_statement(cg, node->children[2], is_main);
            fprintf(cg->output, ".L_end_%d:\n", label_id);
        } else {
            /* if without else */
            fprintf(cg->output, "    je .L_end_%d\n", label_id);
            codegen_statement(cg, node->children[1], is_main);
            fprintf(cg->output, ".L_end_%d:\n", label_id);
        }
    } else if (node->type == AST_WHILE_STATEMENT) {
        int label_id = cg->label_count++;
        cg->break_stack[cg->break_depth].break_label = label_id;
        cg->break_stack[cg->break_depth].continue_label = label_id;
        cg->break_depth++;
        fprintf(cg->output, ".L_while_start_%d:\n", label_id);
        fprintf(cg->output, ".L_continue_%d:\n", label_id);
        codegen_expression(cg, node->children[0]);
        emit_cond_cmp_zero(cg, node->children[0]);
        fprintf(cg->output, "    je .L_while_end_%d\n", label_id);
        codegen_statement(cg, node->children[1], is_main);
        fprintf(cg->output, "    jmp .L_while_start_%d\n", label_id);
        fprintf(cg->output, ".L_while_end_%d:\n", label_id);
        fprintf(cg->output, ".L_break_%d:\n", label_id);
        cg->break_depth--;
    } else if (node->type == AST_DO_WHILE_STATEMENT) {
        /* children: [0]=body, [1]=condition. Body always runs at least once;
         * continue jumps to the condition check, not to the top of the body. */
        int label_id = cg->label_count++;
        cg->break_stack[cg->break_depth].break_label = label_id;
        cg->break_stack[cg->break_depth].continue_label = label_id;
        cg->break_depth++;
        fprintf(cg->output, ".L_do_start_%d:\n", label_id);
        codegen_statement(cg, node->children[0], is_main);
        fprintf(cg->output, ".L_continue_%d:\n", label_id);
        codegen_expression(cg, node->children[1]);
        emit_cond_cmp_zero(cg, node->children[1]);
        fprintf(cg->output, "    jne .L_do_start_%d\n", label_id);
        fprintf(cg->output, ".L_do_end_%d:\n", label_id);
        fprintf(cg->output, ".L_break_%d:\n", label_id);
        cg->break_depth--;
    } else if (node->type == AST_FOR_STATEMENT) {
        /* children: [0]=init, [1]=condition, [2]=update, [3]=body (any may be NULL except body) */
        int label_id = cg->label_count++;
        cg->break_stack[cg->break_depth].break_label = label_id;
        cg->break_stack[cg->break_depth].continue_label = label_id;
        cg->break_depth++;
        /* Save scope: variables declared in the for-init are scoped to the loop. */
        int saved_scope_start = cg->scope_start;
        int saved_local_count = cg->local_count;
        cg->scope_start = cg->local_count;
        if (node->children[0]) {
            /* Stage 76: init may be a declaration or an expression. */
            if (node->children[0]->type == AST_DECLARATION ||
                node->children[0]->type == AST_DECL_LIST) {
                codegen_statement(cg, node->children[0], is_main);
            } else {
                codegen_expression(cg, node->children[0]);
            }
        }
        fprintf(cg->output, ".L_for_start_%d:\n", label_id);
        if (node->children[1]) {
            codegen_expression(cg, node->children[1]);
            emit_cond_cmp_zero(cg, node->children[1]);
            fprintf(cg->output, "    je .L_for_end_%d\n", label_id);
        }
        codegen_statement(cg, node->children[3], is_main);
        fprintf(cg->output, ".L_continue_%d:\n", label_id);
        if (node->children[2]) {
            codegen_expression(cg, node->children[2]);
        }
        fprintf(cg->output, "    jmp .L_for_start_%d\n", label_id);
        fprintf(cg->output, ".L_for_end_%d:\n", label_id);
        fprintf(cg->output, ".L_break_%d:\n", label_id);
        cg->break_depth--;
        /* Pop for-scope variables. */
        cg->local_count = saved_local_count;
        cg->scope_start = saved_scope_start;
    } else if (node->type == AST_SWITCH_STATEMENT) {
        /* children: [0]=controlling expression, [1]=body statement.
         * Pre-walk the body to collect every case/default label
         * (stopping at nested switches), evaluate the expression once,
         * spill it to the stack, emit a linear compare-and-branch
         * dispatch, then emit the body as ordinary statements. When a
         * case/default node is visited during emission it looks up its
         * pre-assigned label via the innermost SwitchCtx. `break`
         * targets the switch-end label. */
        if (cg->switch_depth >= MAX_SWITCH_DEPTH) {
            compile_error( "error: switch nesting exceeds max depth %d\n",
                    MAX_SWITCH_DEPTH);
        }
        int label_id = cg->label_count++;
        SwitchCtx *ctx = &cg->switch_stack[cg->switch_depth];
        ctx->count = 0;
        ctx->default_label = -1;
        ASTNode *body = node->children[1];
        collect_switch_labels(cg, body, ctx);

        codegen_expression(cg, node->children[0]);
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;

        for (int i = 0; i < ctx->count; i++) {
            ASTNode *label_node = ctx->nodes[i];
            if (label_node->type == AST_CASE_SECTION) {
                fprintf(cg->output, "    mov eax, [rsp]\n");
                fprintf(cg->output, "    cmp eax, %s\n",
                        label_node->children[0]->value);
                fprintf(cg->output, "    je .L_switch_sec_%d\n",
                        ctx->labels[i]);
            }
        }
        if (ctx->default_label != -1) {
            fprintf(cg->output, "    jmp .L_switch_sec_%d\n", ctx->default_label);
        } else {
            fprintf(cg->output, "    jmp .L_switch_end_%d\n", label_id);
        }

        cg->break_stack[cg->break_depth].break_label = label_id;
        cg->break_stack[cg->break_depth].continue_label = -1;
        cg->break_depth++;
        cg->switch_depth++;

        codegen_statement(cg, body, is_main);

        cg->switch_depth--;
        cg->break_depth--;

        fprintf(cg->output, ".L_switch_end_%d:\n", label_id);
        fprintf(cg->output, ".L_break_%d:\n", label_id);
        fprintf(cg->output, "    add rsp, 8\n");
        cg->push_depth--;
    } else if (node->type == AST_CASE_SECTION) {
        /* children: [0]=int_literal, [1]=inner statement. The label
         * was pre-assigned by collect_switch_labels when the
         * enclosing switch was emitted; look it up and emit, then
         * fall through to the inner statement. */
        SwitchCtx *ctx = &cg->switch_stack[cg->switch_depth - 1];
        int lbl = switch_lookup_label(ctx, node);
        fprintf(cg->output, ".L_switch_sec_%d:\n", lbl);
        if (node->child_count > 1) {
            codegen_statement(cg, node->children[1], is_main);
        }
    } else if (node->type == AST_DEFAULT_SECTION) {
        /* children: [0]=inner statement. */
        SwitchCtx *ctx = &cg->switch_stack[cg->switch_depth - 1];
        int lbl = switch_lookup_label(ctx, node);
        fprintf(cg->output, ".L_switch_sec_%d:\n", lbl);
        if (node->child_count > 0) {
            codegen_statement(cg, node->children[0], is_main);
        }
    } else if (node->type == AST_LABEL_STATEMENT) {
        fprintf(cg->output, ".L_usr_%s_%s:\n", cg->current_func, node->value);
        if (node->child_count > 0) {
            codegen_statement(cg, node->children[0], is_main);
        }
    } else if (node->type == AST_GOTO_STATEMENT) {
        if (!user_label_defined(cg, node->value)) {
            compile_error( "error: undefined label '%s' in function '%s'\n",
                    node->value, cg->current_func);
        }
        fprintf(cg->output, "    jmp .L_usr_%s_%s\n",
                cg->current_func, node->value);
    } else if (node->type == AST_BREAK_STATEMENT) {
        int id = cg->break_stack[cg->break_depth - 1].break_label;
        fprintf(cg->output, "    jmp .L_break_%d\n", id);
    } else if (node->type == AST_CONTINUE_STATEMENT) {
        /* Walk down the break stack to the nearest loop (switches set
         * continue_label = -1). Parser enforces that `continue` appears
         * only inside a loop, so a valid entry is always found. */
        int id = -1;
        for (int i = cg->break_depth - 1; i >= 0; i--) {
            if (cg->break_stack[i].continue_label != -1) {
                id = cg->break_stack[i].continue_label;
                break;
            }
        }
        fprintf(cg->output, "    jmp .L_continue_%d\n", id);
    } else if (node->type == AST_BLOCK) {
        int saved_scope_start = cg->scope_start;
        int saved_local_count = cg->local_count;
        cg->scope_start = cg->local_count;
        for (int i = 0; i < node->child_count; i++) {
            codegen_statement(cg, node->children[i], is_main);
        }
        /* Pop variables declared in this scope — they are no longer visible. */
        cg->local_count = saved_local_count;
        cg->scope_start = saved_scope_start;
    } else if (node->type == AST_EXPRESSION_STMT) {
        codegen_expression(cg, node->children[0]);
    } else if (node->type == AST_TYPEDEF_DECL) {
        /* Stage 28-01: typedef declarations generate no code. */
    }
}

static void codegen_function(CodeGen *cg, ASTNode *node) {
    if (node->type == AST_FUNCTION_DECL) {
        int is_main = (strcmp(node->value, "main") == 0);

        /* Function children: zero or more AST_PARAM followed by an
         * optional AST_BLOCK body. A pure declaration has no body and
         * emits no code. */
        int num_params = 0;
        while (num_params < node->child_count &&
               node->children[num_params]->type == AST_PARAM) {
            num_params++;
        }
        if (num_params == node->child_count) {
            return;
        }
        ASTNode *body = node->children[num_params];

        /* Reset per-function symbol table */
        cg->local_count = 0;
        cg->stack_offset = 0;
        cg->scope_start = 0;
        cg->push_depth = 0;
        cg->user_label_count = 0;
        cg->current_func = node->value;
        cg->current_return_type = node->decl_type;
        cg->current_return_full_type =
            (node->decl_type == TYPE_POINTER) ? node->full_type : NULL;

        /* Pre-walk the body to collect user labels; rejects duplicates. */
        collect_user_labels(cg, body);

        /* Compute stack space: 8 bytes per parameter (conservative
         * upper bound covering long plus worst-case alignment) plus a
         * conservative 8-byte upper bound per body declaration.
         * Stage 75-04: variadic functions also reserve 304 bytes for the
         * hidden GP/FP register save area.
         * Rounded up to a 16-byte multiple. */
        int stack_size = num_params * 8 + compute_decl_bytes(body);
        if (node->is_variadic)
            stack_size += 304;
        if (stack_size % 16 != 0)
            stack_size = (stack_size + 15) & ~15;

        /* Function label and prologue. Stage 14-07: always emit
         * `push rbp; mov rbp, rsp` so rsp is 16-byte aligned inside
         * the function regardless of how it was entered. With the
         * stage-14-07 link change (gcc -no-pie), `_start` calls
         * `main` so rsp is 8 mod 16 at main entry; the unconditional
         * push rbp restores 16-byte alignment that SysV AMD64
         * requires at every internal call site (notably libc calls
         * such as `puts` that use SSE-aligned loads). The
         * `sub rsp, N` is still elided when there are no locals. */
        /* Stage 23: static functions have internal linkage — omit `global`. */
        if (node->storage_class != SC_STATIC)
            fprintf(cg->output, "global %s\n", node->value);
        fprintf(cg->output, "%s:\n", node->value);
        cg->has_frame = 1;
        fprintf(cg->output, "    push rbp\n");
        fprintf(cg->output, "    mov rbp, rsp\n");
        if (stack_size > 0) {
            fprintf(cg->output, "    sub rsp, %d\n", stack_size);
        }

        /* Stage 75-04: variadic function register save area.
         * Reserve 304 bytes at the top of the frame (before named params) and
         * save all 6 GP argument registers so va_start can reference them. */
        if (node->is_variadic) {
            cg->stack_offset += 304;
            cg->variadic_reg_save_offset    = cg->stack_offset;
            cg->variadic_named_gp_params    = num_params < 6 ? num_params : 6;
            cg->variadic_named_stack_params = num_params > 6 ? num_params - 6 : 0;
            int rso = cg->variadic_reg_save_offset;
            fprintf(cg->output, "    mov [rbp - %d], rdi\n", rso);
            fprintf(cg->output, "    mov [rbp - %d], rsi\n", rso - 8);
            fprintf(cg->output, "    mov [rbp - %d], rdx\n", rso - 16);
            fprintf(cg->output, "    mov [rbp - %d], rcx\n", rso - 24);
            fprintf(cg->output, "    mov [rbp - %d], r8\n",  rso - 32);
            fprintf(cg->output, "    mov [rbp - %d], r9\n",  rso - 40);
        } else {
            cg->variadic_reg_save_offset    = 0;
            cg->variadic_named_gp_params    = 0;
            cg->variadic_named_stack_params = 0;
        }

        /*
         * Parameters share the outermost body scope (scope_start stays at 0).
         * A body-level declaration that collides with a parameter name is
         * rejected by the existing duplicate-detection check in
         * codegen_statement for AST_DECLARATION.
         *
         * Each parameter's stack slot is sized for its declared type;
         * the store uses the correspondingly-sized sub-register of the
         * SysV AMD64 argument register so the full declared width is
         * preserved and nothing above it is touched.
         */
        static const char *param_regs_8[6]  = {
            "dil", "sil", "dl",  "cl",  "r8b", "r9b"
        };
        static const char *param_regs_16[6] = {
            "di",  "si",  "dx",  "cx",  "r8w", "r9w"
        };
        static const char *param_regs_32[6] = {
            "edi", "esi", "edx", "ecx", "r8d", "r9d"
        };
        static const char *param_regs_64[6] = {
            "rdi", "rsi", "rdx", "rcx", "r8",  "r9"
        };
        int reg_params = num_params < 6 ? num_params : 6;
        for (int i = 0; i < reg_params; i++) {
            /* Stage 75-01: unnamed params in variadic definitions are ignored. */
            if (node->children[i]->value[0] == '\0') continue;
            TypeKind pt = node->children[i]->decl_type;
            int sz = type_kind_bytes(pt);
            int offset = codegen_add_var(cg, node->children[i]->value, sz, sz,
                                         pt, node->children[i]->full_type);
            cg->locals[cg->local_count - 1].is_unsigned = node->children[i]->is_unsigned;
            const char *reg;
            switch (sz) {
            case 1: reg = param_regs_8[i];  break;
            case 2: reg = param_regs_16[i]; break;
            case 8: reg = param_regs_64[i]; break;
            case 4:
            default: reg = param_regs_32[i]; break;
            }
            fprintf(cg->output, "    mov [rbp - %d], %s\n", offset, reg);
        }
        /* Stage 68: params 7+ arrive as stack arguments at [rbp + 16 + (i-6)*8].
         * Copy each into a local slot so the rest of codegen uses uniform
         * negative-rbp-offset access for all parameters. */
        for (int i = 6; i < num_params; i++) {
            /* Stage 75-01: unnamed params in variadic definitions are ignored. */
            if (node->children[i]->value[0] == '\0') continue;
            TypeKind pt = node->children[i]->decl_type;
            int sz = type_kind_bytes(pt);
            int offset = codegen_add_var(cg, node->children[i]->value, sz, sz,
                                         pt, node->children[i]->full_type);
            cg->locals[cg->local_count - 1].is_unsigned = node->children[i]->is_unsigned;
            int src = 16 + (i - 6) * 8;
            switch (sz) {
            case 1:
                if (node->children[i]->is_unsigned)
                    fprintf(cg->output, "    movzx eax, byte [rbp + %d]\n", src);
                else
                    fprintf(cg->output, "    movsx eax, byte [rbp + %d]\n", src);
                break;
            case 2:
                if (node->children[i]->is_unsigned)
                    fprintf(cg->output, "    movzx eax, word [rbp + %d]\n", src);
                else
                    fprintf(cg->output, "    movsx eax, word [rbp + %d]\n", src);
                break;
            case 8:
                fprintf(cg->output, "    mov rax, [rbp + %d]\n", src);
                break;
            case 4:
            default:
                fprintf(cg->output, "    mov eax, [rbp + %d]\n", src);
                break;
            }
            emit_store_local(cg, offset, sz, sz == 8 ? 1 : 0);
        }

        /* Generate body statements directly — the function body acts as the outermost scope. */
        for (int i = 0; i < body->child_count; i++) {
            codegen_statement(cg, body->children[i], is_main);
        }

        /* Void functions get an implicit epilogue so falling off the end
         * returns cleanly.  For non-void functions the behaviour of
         * falling off the end is undefined; we don't emit a spurious ret. */
        if (node->decl_type == TYPE_VOID) {
            fprintf(cg->output, "    mov rsp, rbp\n");
            fprintf(cg->output, "    pop rbp\n");
            fprintf(cg->output, "    ret\n");
        }
    }
}

/*
 * Stage 14-03: emit the static-data section after every function has
 * been written. Each pooled AST_STRING_LITERAL becomes a `Lstr<N>`
 * label followed by a `db` line listing the payload bytes followed by
 * a NUL terminator. Empty literals collapse to `db 0`. The section is
 * skipped entirely when no literals were collected.
 */
static void codegen_emit_string_pool(CodeGen *cg) {
    if (cg->string_pool_count == 0) return;
    fprintf(cg->output, "section .rodata\n");
    for (int i = 0; i < cg->string_pool_count; i++) {
        ASTNode *s = cg->string_pool[i];
        /* Stage 14-05: byte count is taken from the AST node's
         * byte_length, which the parser stamps from the lexer's
         * decoded count. This is required because the decoded payload
         * may contain embedded NULs (from the `\0` escape) and
         * `full_type` was already rewritten to `char *` during the
         * expression-time array-to-pointer decay. */
        int byte_len = s->byte_length;
        fprintf(cg->output, "Lstr%d:\n", i);
        fprintf(cg->output, "    db ");
        for (int j = 0; j < byte_len; j++) {
            fprintf(cg->output, "%d, ", (unsigned char)s->value[j]);
        }
        fprintf(cg->output, "0\n");
    }
}

/*
 * Stage 14-07: a function whose AST_FUNCTION_DECL has no trailing
 * AST_BLOCK is a pure declaration (parameters only). Used to decide
 * whether to emit an `extern <name>` directive for the linker.
 */
static int function_has_body(ASTNode *func) {
    if (!func) return 0;
    for (int i = func->child_count - 1; i >= 0; i--) {
        if (func->children[i]->type == AST_BLOCK) return 1;
    }
    return 0;
}

/*
 * Stage 14-07: scan the translation unit for an AST_FUNCTION_DECL
 * with the given name that carries a body. Used to suppress
 * `extern` emission for functions that have a forward declaration
 * AND a definition in the same TU.
 */
static int tu_has_definition_for(ASTNode *tu, const char *name) {
    if (!tu) return 0;
    for (int i = 0; i < tu->child_count; i++) {
        ASTNode *c = tu->children[i];
        if (c->type == AST_FUNCTION_DECL &&
            strcmp(c->value, name) == 0 &&
            function_has_body(c)) {
            return 1;
        }
    }
    return 0;
}

/*
 * Stage 14-07: emit `extern <name>` directives for every function
 * that is declared but never defined in this translation unit, so
 * NASM marks the symbol as undefined and the linker resolves it
 * (e.g. against libc's `puts`). Multiple non-defining declarations
 * of the same name collapse to a single `extern` line.
 */
static void codegen_emit_externs(CodeGen *cg, ASTNode *tu) {
    for (int i = 0; i < tu->child_count; i++) {
        ASTNode *c = tu->children[i];
        if (c->type != AST_FUNCTION_DECL) continue;
        if (function_has_body(c)) continue;
        if (tu_has_definition_for(tu, c->value)) continue;
        int already_emitted = 0;
        for (int k = 0; k < i; k++) {
            ASTNode *p = tu->children[k];
            if (p->type == AST_FUNCTION_DECL &&
                !function_has_body(p) &&
                strcmp(p->value, c->value) == 0) {
                already_emitted = 1;
                break;
            }
        }
        if (already_emitted) continue;
        fprintf(cg->output, "extern %s\n", c->value);
    }
}

/*
 * Stage 22-01: map a TypeKind to its NASM .bss reserve directive mnemonic.
 * Pointer and long use resq (8 bytes); int uses resd (4 bytes); short uses
 * resw (2 bytes); char uses resb (1 byte).
 */
static const char *bss_res_directive(TypeKind kind) {
    switch (kind) {
    case TYPE_CHAR:               return "resb";
    case TYPE_SHORT:              return "resw";
    case TYPE_LONG_LONG:
    case TYPE_UNSIGNED_LONG_LONG:
    case TYPE_LONG:
    case TYPE_POINTER:            return "resq";
    case TYPE_INT:
    default:                      return "resd";
    }
}

/*
 * Stage 22-01/22-02: register a file-scope AST_DECLARATION in the global table.
 * Stage 22-02: if the declaration has a child node it carries a constant
 * initializer; the value is extracted and the global is marked initialized
 * (→ .data).  Uninitialized globals (→ .bss) have is_initialized == 0.
 */
static void codegen_add_global(CodeGen *cg, ASTNode *decl) {
    if (cg->global_count >= MAX_GLOBALS) {
        compile_error( "error: too many global variables (max %d)\n", MAX_GLOBALS);
    }
    GlobalVar *gv = &cg->globals[cg->global_count];
    strncpy(gv->name, decl->value, 255);
    gv->name[255] = '\0';
    gv->kind = decl->decl_type;
    gv->full_type = decl->full_type;
    if (decl->decl_type == TYPE_ARRAY && decl->full_type) {
        gv->size = decl->full_type->base
                   ? type_kind_bytes(decl->full_type->base->kind)
                   : 4;
    } else {
        gv->size = type_kind_bytes(decl->decl_type);
    }
    gv->is_initialized = 0;
    gv->init_value = 0;
    gv->is_label_init = 0;
    gv->init_label[0] = '\0';
    gv->is_const = decl->is_const;
    gv->is_unsigned = decl->is_unsigned;
    gv->init_node = NULL;
    if (decl->child_count > 0) {
        ASTNode *init = decl->children[0];
        if (init->type == AST_INT_LITERAL) {
            long v = strtol(init->value, NULL, 10);
            gv->init_value = (gv->kind == TYPE_BOOL) ? (v != 0 ? 1 : 0) : v;
            gv->is_initialized = 1;
        } else if (init->type == AST_CHAR_LITERAL) {
            long v = (long)(unsigned char)init->value[0];
            gv->init_value = (gv->kind == TYPE_BOOL) ? (v != 0 ? 1 : 0) : v;
            gv->is_initialized = 1;
        } else if (init->type == AST_VAR_REF) {
            /* Stage 25-02: function-pointer global initialized from a
             * function designator — store the symbol name for .data emit. */
            strncpy(gv->init_label, init->value, 255);
            gv->init_label[255] = '\0';
            gv->is_label_init = 1;
            gv->is_initialized = 1;
        } else if (gv->kind == TYPE_ARRAY && init->type == AST_STRING_LITERAL) {
            /* Stage 43: char s[] = "abc" — char array from string literal. */
            gv->init_node = init;
            gv->is_initialized = 1;
        } else if (gv->kind == TYPE_POINTER && init->type == AST_STRING_LITERAL) {
            /* Stage 43: char *s = "abc" — pointer initialized from string
             * literal.  Add the literal to the string pool now so its label
             * is assigned before codegen_emit_data runs. */
            if (cg->string_pool_count >= MAX_STRING_LITERALS) {
                compile_error( "error: too many string literals (max %d)\n",
                        MAX_STRING_LITERALS);
            }
            int idx = cg->string_pool_count;
            cg->string_pool[idx] = init;
            cg->string_pool_count++;
            snprintf(gv->init_label, sizeof(gv->init_label), "Lstr%d", idx);
            gv->is_label_init = 1;
            gv->is_initialized = 1;
        } else if (gv->kind == TYPE_ARRAY && init->type == AST_INITIALIZER_LIST) {
            /* Stage 43: int values[] = {10,20,30} or char *names[] = {...}. */
            gv->init_node = init;
            gv->is_initialized = 1;
        } else if ((gv->kind == TYPE_STRUCT || gv->kind == TYPE_UNION) &&
                   init->type == AST_INITIALIZER_LIST) {
            /* Stage 44/72: struct/union global initialized from brace list. */
            gv->init_node = init;
            gv->is_initialized = 1;
        }
    }
    cg->global_count++;
}

/*
 * Stage 22-02: map a TypeKind to its NASM .data initialized directive.
 */
static const char *data_init_directive(TypeKind kind) {
    switch (kind) {
    case TYPE_BOOL:
    case TYPE_CHAR:               return "db";
    case TYPE_SHORT:              return "dw";
    case TYPE_LONG_LONG:
    case TYPE_UNSIGNED_LONG_LONG:
    case TYPE_LONG:
    case TYPE_POINTER:            return "dq";
    case TYPE_INT:
    default:                      return "dd";
    }
}

/*
 * Stage 44: emit .data bytes for a struct value from a brace-list.
 * Recursive: nested struct fields recurse; pointer fields from string
 * literals are added to the pool and emitted as dq Lstr<N>; scalar
 * fields emit the literal value; missing trailing fields emit zero.
 * The label for the whole object has already been emitted by the caller.
 */
static void emit_global_struct(CodeGen *cg, Type *st, ASTNode *list) {
    int ninit = list ? list->child_count : 0;
    int cur_off = 0; /* bytes emitted so far within this struct */
    for (int i = 0; i < st->field_count; i++) {
        StructField *f = &st->fields[i];
        int fsize = f->full_type ? f->full_type->size : type_kind_bytes(f->kind);
        /* Emit padding bytes to reach the field's declared offset. */
        while (cur_off < f->offset) {
            fprintf(cg->output, "    db 0\n");
            cur_off++;
        }
        if (i < ninit) {
            ASTNode *elem = list->children[i];
            if (f->kind == TYPE_STRUCT && f->full_type &&
                elem->type == AST_INITIALIZER_LIST) {
                emit_global_struct(cg, f->full_type, elem);
            } else if (f->kind == TYPE_POINTER &&
                       elem->type == AST_STRING_LITERAL) {
                if (cg->string_pool_count >= MAX_STRING_LITERALS) {
                    compile_error( "error: too many string literals (max %d)\n",
                            MAX_STRING_LITERALS);
                }
                int idx = cg->string_pool_count;
                cg->string_pool[idx] = elem;
                cg->string_pool_count++;
                fprintf(cg->output, "    dq Lstr%d\n", idx);
            } else if (elem->type == AST_INT_LITERAL) {
                long v = strtol(elem->value, NULL, 10);
                fprintf(cg->output, "    %s %ld\n",
                        data_init_directive(f->kind), v);
            } else if (elem->type == AST_CHAR_LITERAL) {
                long v = (long)(unsigned char)elem->value[0];
                fprintf(cg->output, "    %s %ld\n",
                        data_init_directive(f->kind), v);
            } else {
                compile_error(
                        "error: unsupported initializer for struct field '%s'\n",
                        f->name);
            }
        } else {
            /* Zero-fill missing trailing fields. */
            for (int b = 0; b < fsize; b++)
                fprintf(cg->output, "    db 0\n");
        }
        cur_off = f->offset + fsize;
    }
    /* Emit trailing padding to reach the struct's total size. */
    while (cur_off < st->size) {
        fprintf(cg->output, "    db 0\n");
        cur_off++;
    }
}

/*
 * Stage 22-02: emit section .data for all initialized globals.
 * Emits nothing when no globals are initialized.
 */
static void codegen_emit_data(CodeGen *cg) {
    int has_data = 0;
    for (int i = 0; i < cg->global_count; i++) {
        if (cg->globals[i].is_initialized) { has_data = 1; break; }
    }
    if (!has_data) return;
    fprintf(cg->output, "section .data\n");
    for (int i = 0; i < cg->global_count; i++) {
        GlobalVar *gv = &cg->globals[i];
        if (!gv->is_initialized) continue;

        if (gv->init_node && gv->kind == TYPE_ARRAY &&
            gv->init_node->type == AST_STRING_LITERAL) {
            /* Stage 43: char s[] = "abc" — emit bytes inline. */
            ASTNode *str = gv->init_node;
            int arr_len = gv->full_type ? gv->full_type->length : str->byte_length + 1;
            fprintf(cg->output, "%s: db ", gv->name);
            for (int j = 0; j < arr_len; j++) {
                unsigned char b = (j < str->byte_length)
                                  ? (unsigned char)str->value[j] : 0;
                if (j > 0) fprintf(cg->output, ", ");
                fprintf(cg->output, "%d", b);
            }
            fprintf(cg->output, "\n");
        } else if (gv->init_node &&
                   (gv->kind == TYPE_STRUCT || gv->kind == TYPE_UNION) &&
                   gv->init_node->type == AST_INITIALIZER_LIST) {
            /* Stage 44/72: struct/union global initialized from brace-list. */
            fprintf(cg->output, "%s:\n", gv->name);
            if (gv->kind == TYPE_UNION) {
                /* Union: initialize first member, zero-fill the rest. */
                ASTNode *list = gv->init_node;
                int cur_off = 0;
                if (list->child_count > 1) {
                    compile_error( "error: too many initializers for union '%s'\n",
                            gv->name);
                }
                if (list->child_count == 1 && gv->full_type->field_count > 0) {
                    StructField *f = &gv->full_type->fields[0];
                    int fsize = f->full_type ? f->full_type->size : type_kind_bytes(f->kind);
                    ASTNode *elem = list->children[0];
                    if (elem->type == AST_INT_LITERAL) {
                        long v = strtol(elem->value, NULL, 10);
                        fprintf(cg->output, "    %s %ld\n", data_init_directive(f->kind), v);
                    } else if (elem->type == AST_CHAR_LITERAL) {
                        long v = (long)(unsigned char)elem->value[0];
                        fprintf(cg->output, "    %s %ld\n", data_init_directive(f->kind), v);
                    } else {
                        compile_error( "error: unsupported initializer for union '%s'\n",
                                gv->name);
                    }
                    cur_off = fsize;
                }
                while (cur_off < gv->full_type->size) {
                    fprintf(cg->output, "    db 0\n");
                    cur_off++;
                }
            } else {
                emit_global_struct(cg, gv->full_type, gv->init_node);
            }
        } else if (gv->init_node && gv->kind == TYPE_ARRAY &&
                   gv->init_node->type == AST_INITIALIZER_LIST) {
            /* Stage 43: int values[] = {10,20,30} or char *names[] = {...}.
             * Stage 44: struct Point pts[] = {{1,2},{10,20},...}.
             * One directive per element; struct elements delegate to
             * emit_global_struct. */
            ASTNode *list = gv->init_node;
            int arr_len = gv->full_type ? gv->full_type->length : list->child_count;
            Type *elem_type = gv->full_type ? gv->full_type->base : NULL;
            TypeKind elem_kind = elem_type ? elem_type->kind : TYPE_INT;
            const char *dir = data_init_directive(elem_kind);
            for (int j = 0; j < arr_len; j++) {
                /* Emit label on first element only. */
                if (j == 0) fprintf(cg->output, "%s:\n", gv->name);
                if (j < list->child_count) {
                    ASTNode *elem = list->children[j];
                    if (elem_type && elem_type->kind == TYPE_STRUCT &&
                        elem->type == AST_INITIALIZER_LIST) {
                        /* Stage 44: struct element. */
                        emit_global_struct(cg, elem_type, elem);
                    } else if (elem->type == AST_STRING_LITERAL) {
                        /* char *names[] element — add to pool, emit dq Lstr<N>. */
                        if (cg->string_pool_count >= MAX_STRING_LITERALS) {
                            compile_error( "error: too many string literals (max %d)\n",
                                    MAX_STRING_LITERALS);
                        }
                        int idx = cg->string_pool_count;
                        cg->string_pool[idx] = elem;
                        cg->string_pool_count++;
                        fprintf(cg->output, "    dq Lstr%d\n", idx);
                    } else if (elem->type == AST_INT_LITERAL) {
                        long v = strtol(elem->value, NULL, 10);
                        fprintf(cg->output, "    %s %ld\n", dir, v);
                    } else if (elem->type == AST_CHAR_LITERAL) {
                        long v = (long)(unsigned char)elem->value[0];
                        fprintf(cg->output, "    %s %ld\n", dir, v);
                    } else {
                        compile_error(
                                "error: unsupported initializer element type in '%s'\n",
                                gv->name);
                    }
                } else {
                    /* Zero-fill trailing elements. */
                    if (elem_type && elem_type->kind == TYPE_STRUCT) {
                        for (int b = 0; b < elem_type->size; b++)
                            fprintf(cg->output, "    db 0\n");
                    } else {
                        fprintf(cg->output, "    %s 0\n", dir);
                    }
                }
            }
        } else if (gv->is_label_init) {
            /* Stage 25-02 / Stage 43: label-reference initializer. */
            fprintf(cg->output, "%s: %s %s\n",
                    gv->name, data_init_directive(gv->kind), gv->init_label);
        } else {
            fprintf(cg->output, "%s: %s %ld\n",
                    gv->name, data_init_directive(gv->kind), gv->init_value);
        }
    }
}

static void codegen_emit_bss(CodeGen *cg) {
    int has_bss = 0;
    for (int i = 0; i < cg->global_count; i++) {
        if (!cg->globals[i].is_initialized) { has_bss = 1; break; }
    }
    if (!has_bss) return;

    fprintf(cg->output, "section .bss\n");
    for (int i = 0; i < cg->global_count; i++) {
        GlobalVar *gv = &cg->globals[i];
        if (gv->is_initialized) continue;
        if (gv->kind == TYPE_ARRAY && gv->full_type) {
            fprintf(cg->output, "%s: %s %d\n",
                    gv->name,
                    bss_res_directive(gv->full_type->base->kind),
                    gv->full_type->length);
        } else if ((gv->kind == TYPE_STRUCT || gv->kind == TYPE_UNION) &&
                   gv->full_type) {
            /* Reserve the exact byte count for struct/union globals. */
            fprintf(cg->output, "%s: resb %d\n", gv->name, gv->full_type->size);
        } else {
            fprintf(cg->output, "%s: %s 1\n",
                    gv->name, bss_res_directive(gv->kind));
        }
    }
}

/* Stage 71: emit block-scope static variables accumulated during function
 * body emission. Initialized statics go to .data; uninitialized to .bss.
 * These sections are separate from the file-scope globals emitted earlier
 * and are merged by the assembler. */
static void codegen_emit_local_statics(CodeGen *cg) {
    int has_data = 0, has_bss = 0;
    for (int i = 0; i < cg->local_static_count; i++) {
        if (cg->local_statics[i].is_initialized) has_data = 1;
        else has_bss = 1;
    }
    if (has_data) {
        fprintf(cg->output, "section .data\n");
        for (int i = 0; i < cg->local_static_count; i++) {
            LocalStaticVar *sv = &cg->local_statics[i];
            if (!sv->is_initialized) continue;
            fprintf(cg->output, "%s: %s %ld\n",
                    sv->label, data_init_directive(sv->kind), sv->init_value);
        }
    }
    if (has_bss) {
        fprintf(cg->output, "section .bss\n");
        for (int i = 0; i < cg->local_static_count; i++) {
            LocalStaticVar *sv = &cg->local_statics[i];
            if (sv->is_initialized) continue;
            fprintf(cg->output, "%s: %s 1\n",
                    sv->label, bss_res_directive(sv->kind));
        }
    }
}

void codegen_translation_unit(CodeGen *cg, ASTNode *node) {
    cg->tu_root = node;
    if (node->type == AST_TRANSLATION_UNIT) {
        /* Stage 22-01/22-02: first pass — register all file-scope globals.
         * AST_DECL_LIST (from comma-separated declarations) is expanded
         * so each individual AST_DECLARATION is registered in turn.
         * Stage 23: extern-only declarations (no definition in this TU)
         * are skipped — they allocate no storage. */
        for (int i = 0; i < node->child_count; i++) {
            ASTNode *child = node->children[i];
            if (child->type == AST_DECLARATION) {
                if (child->storage_class != SC_EXTERN)
                    codegen_add_global(cg, child);
            } else if (child->type == AST_DECL_LIST) {
                for (int j = 0; j < child->child_count; j++) {
                    ASTNode *d = child->children[j];
                    if (d->storage_class != SC_EXTERN)
                        codegen_add_global(cg, d);
                }
            }
        }
        codegen_emit_data(cg);
        codegen_emit_bss(cg);
    }
    fprintf(cg->output, "section .text\n");
    if (node->type == AST_TRANSLATION_UNIT) {
        codegen_emit_externs(cg, node);
        for (int i = 0; i < node->child_count; i++) {
            codegen_function(cg, node->children[i]);
        }
    }
    codegen_emit_string_pool(cg);
    /* Stage 71: emit block-scope static variable storage accumulated
     * during function body emission. */
    codegen_emit_local_statics(cg);
    /* Mark the stack as non-executable so the linker doesn't warn about a
     * missing .note.GNU-stack section (ELF ABI requirement on Linux). */
    fprintf(cg->output, "\nsection .note.GNU-stack noalloc noexec nowrite progbits\n");
}
