#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "type.h"

static int type_kind_bytes(TypeKind kind) {
    switch (kind) {
    case TYPE_CHAR:    return 1;
    case TYPE_SHORT:   return 2;
    case TYPE_INT:     return 4;
    case TYPE_LONG:    return 8;
    case TYPE_POINTER: return 8;
    case TYPE_ARRAY:   return 0; /* size lives on full_type; caller uses that */
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
 * char/short sign-extend into eax (implicit zero-extend into rax);
 * int loads into eax (implicit zero-extend into rax); long loads
 * the full 8-byte slot into rax.
 */
static void emit_load_local(CodeGen *cg, int offset, int size) {
    switch (size) {
    case 1: fprintf(cg->output, "    movsx eax, byte [rbp - %d]\n", offset); break;
    case 2: fprintf(cg->output, "    movsx eax, word [rbp - %d]\n", offset); break;
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
 * int (src_is_long=0). For an 8-byte destination with a 32-bit source
 * the value is sign-extended via movsxd before the store so the full
 * slot is written with a correct signed value.
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
                fprintf(stderr, "error: duplicate label '%s' in function '%s'\n",
                        node->value, cg->current_func);
                exit(1);
            }
        }
        if (cg->user_label_count >= MAX_USER_LABELS) {
            fprintf(stderr, "error: too many labels in function '%s' (max %d)\n",
                    cg->current_func, MAX_USER_LABELS);
            exit(1);
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
            fprintf(stderr, "error: too many case/default labels in switch (max %d)\n",
                    MAX_SWITCH_LABELS);
            exit(1);
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
            fprintf(stderr, "error: too many case/default labels in switch (max %d)\n",
                    MAX_SWITCH_LABELS);
            exit(1);
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
    switch (lv->kind) {
    case TYPE_CHAR:  return type_char();
    case TYPE_SHORT: return type_short();
    case TYPE_LONG:  return type_long();
    case TYPE_INT:
    default:         return type_int();
    }
}

static void codegen_expression(CodeGen *cg, ASTNode *node);

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
    if (base_node->type != AST_VAR_REF) {
        fprintf(stderr, "error: subscript base must be an identifier\n");
        exit(1);
    }
    LocalVar *lv = codegen_find_var(cg, base_node->value);
    if (!lv) {
        fprintf(stderr, "error: undeclared variable '%s'\n", base_node->value);
        exit(1);
    }
    Type *element;
    if (lv->kind == TYPE_ARRAY) {
        element = lv->full_type->base;
        fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset);
    } else if (lv->kind == TYPE_POINTER) {
        element = lv->full_type->base;
        fprintf(cg->output, "    mov rax, [rbp - %d]\n", lv->offset);
    } else {
        fprintf(stderr,
                "error: subscript base '%s' is not an array or pointer\n",
                base_node->value);
        exit(1);
    }
    int elem_size = type_size(element);

    fprintf(cg->output, "    push rax\n");
    cg->push_depth++;
    codegen_expression(cg, index_node);
    TypeKind index_kind = index_node->result_type;
    if (index_kind != TYPE_INT && index_kind != TYPE_LONG) {
        fprintf(stderr, "error: array subscript index must be an integer\n");
        exit(1);
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
    return (t == TYPE_LONG) ? TYPE_LONG : TYPE_INT;
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
 * Map a local's storage size to its post-promotion type. A size-8
 * local is long; any smaller size promotes to int when used in an
 * expression (sign-extended on load).
 */
static TypeKind type_kind_from_size(int size) {
    return (size == 8) ? TYPE_LONG : TYPE_INT;
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
        if (lv && (lv->kind == TYPE_POINTER || lv->kind == TYPE_ARRAY)) {
            /* Stage 13-03: an array name in a value context decays to
             * a pointer to its element type. */
            t = TYPE_POINTER;
        } else {
            t = lv ? promote_kind(type_kind_from_size(lv->size)) : TYPE_INT;
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
    default:
        t = TYPE_INT;
        break;
    }
    node->result_type = t;
    return t;
}

static void codegen_expression(CodeGen *cg, ASTNode *node) {
    if (node->type == AST_INT_LITERAL) {
        if (node->decl_type == TYPE_LONG) {
            fprintf(cg->output, "    mov rax, %s\n", node->value);
            node->result_type = TYPE_LONG;
        } else {
            fprintf(cg->output, "    mov eax, %s\n", node->value);
            node->result_type = TYPE_INT;
        }
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
            fprintf(stderr,
                    "error: too many string literals (max %d)\n",
                    MAX_STRING_LITERALS);
            exit(1);
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
        if (!lv) {
            fprintf(stderr, "error: undeclared variable '%s'\n", node->value);
            exit(1);
        }
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
        }
        emit_load_local(cg, lv->offset, lv->size);
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
                fprintf(stderr, "error: cannot dereference non-pointer value\n");
                exit(1);
            }
            Type *base = operand_type->base;
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
        if (!lv) {
            fprintf(stderr, "error: undeclared variable '%s'\n", node->value);
            exit(1);
        }
        /* Stage 13-01: arrays are not assignable. */
        if (lv->kind == TYPE_ARRAY) {
            fprintf(stderr, "error: arrays are not assignable\n");
            exit(1);
        }
        codegen_expression(cg, node->children[0]);
        /* Pointer RHS values are already in full rax — skip the
         * 32-to-64 sign-extend that integer values would need. */
        int rhs_is_long = (node->children[0]->result_type == TYPE_LONG ||
                           node->children[0]->result_type == TYPE_POINTER);
        emit_store_local(cg, lv->offset, lv->size, rhs_is_long);
        if (lv->kind == TYPE_POINTER) {
            node->result_type = TYPE_POINTER;
            node->full_type = lv->full_type;
        } else {
            node->result_type = type_kind_from_size(lv->size);
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
        if (!lv) {
            fprintf(stderr, "error: undeclared variable '%s'\n", operand->value);
            exit(1);
        }
        fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset);
        node->result_type = TYPE_POINTER;
        node->full_type = type_pointer(local_var_type(lv));
        return;
    }
    if (node->type == AST_DEREF) {
        /* Evaluate the pointer expression — its value (the address)
         * lands in rax. Operand must be of pointer type; load through
         * the address using the base type's width. */
        codegen_expression(cg, node->children[0]);
        Type *operand_type = node->children[0]->full_type;
        if (!operand_type || operand_type->kind != TYPE_POINTER) {
            fprintf(stderr, "error: cannot dereference non-pointer value\n");
            exit(1);
        }
        Type *base = operand_type->base;
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
                fprintf(stderr,
                        "error: operator '!' not supported on pointer operands\n");
                exit(1);
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
                fprintf(stderr,
                        "error: operator '~' not supported on pointer operands\n");
                exit(1);
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
        /* ++x or --x: update variable, result is new value. Arithmetic
         * stays 32-bit this stage; size-aware store/load preserves the
         * declared width. */
        const char *var_name = node->children[0]->value;
        LocalVar *lv = codegen_find_var(cg, var_name);
        if (!lv) {
            fprintf(stderr, "error: undeclared variable '%s'\n", var_name);
            exit(1);
        }
        emit_load_local(cg, lv->offset, lv->size);
        if (strcmp(node->value, "++") == 0) {
            fprintf(cg->output, "    add eax, 1\n");
        } else {
            fprintf(cg->output, "    sub eax, 1\n");
        }
        emit_store_local(cg, lv->offset, lv->size, 0);
        node->result_type = TYPE_INT;
        return;
    }
    if (node->type == AST_POSTFIX_INC_DEC) {
        /* x++ or x--: result is old value, then update variable */
        const char *var_name = node->children[0]->value;
        LocalVar *lv = codegen_find_var(cg, var_name);
        if (!lv) {
            fprintf(stderr, "error: undeclared variable '%s'\n", var_name);
            exit(1);
        }
        emit_load_local(cg, lv->offset, lv->size);
        fprintf(cg->output, "    mov ecx, eax\n");  /* save old value */
        if (strcmp(node->value, "++") == 0) {
            fprintf(cg->output, "    add eax, 1\n");
        } else {
            fprintf(cg->output, "    sub eax, 1\n");
        }
        emit_store_local(cg, lv->offset, lv->size, 0);
        fprintf(cg->output, "    mov eax, ecx\n");  /* restore old value as result */
        node->result_type = TYPE_INT;
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
        /* Resolve the callee so each argument can be converted to its
         * declared parameter type before being passed. The parser has
         * already validated that the callee exists and its parameter
         * count matches `nargs`. */
        ASTNode *callee = codegen_find_function_decl(cg, node->value);
        /* Evaluate arguments left-to-right, converting each to the
         * callee's declared parameter type, then pushing the result.
         * Stage 12-04: when either side is a pointer, the integer
         * widen/narrow conversion does not apply — the address is
         * already in the full rax. Instead enforce strict pointer/
         * integer matching and pointer-base-type equality. */
        for (int i = 0; i < nargs; i++) {
            codegen_expression(cg, node->children[i]);
            if (callee && i < callee->child_count &&
                callee->children[i]->type == AST_PARAM) {
                ASTNode *param = callee->children[i];
                TypeKind src = node->children[i]->result_type;
                TypeKind dst = param->decl_type;
                if (dst == TYPE_POINTER || src == TYPE_POINTER) {
                    if (dst != TYPE_POINTER) {
                        fprintf(stderr,
                                "error: function '%s' parameter '%s' expected integer argument, got pointer\n",
                                node->value, param->value);
                        exit(1);
                    }
                    if (src != TYPE_POINTER) {
                        fprintf(stderr,
                                "error: function '%s' parameter '%s' expected pointer argument, got integer\n",
                                node->value, param->value);
                        exit(1);
                    }
                    if (!pointer_types_equal(node->children[i]->full_type,
                                             param->full_type)) {
                        fprintf(stderr,
                                "error: function '%s' parameter '%s' has incompatible pointer type\n",
                                node->value, param->value);
                        exit(1);
                    }
                } else {
                    emit_convert(cg, src, dst);
                }
            }
            fprintf(cg->output, "    push rax\n");
            cg->push_depth++;
        }
        /* Pop into argument registers in reverse order. */
        for (int i = nargs - 1; i >= 0; i--) {
            fprintf(cg->output, "    pop %s\n", arg_regs[i]);
            cg->push_depth--;
        }
        /* SysV AMD64 requires rsp 16-aligned at the call. The frame prologue
         * leaves rsp 16-aligned; each live `push rax` offsets it by 8. */
        int needs_pad = (cg->push_depth % 2) != 0;
        if (needs_pad) {
            fprintf(cg->output, "    sub rsp, 8\n");
        }
        fprintf(cg->output, "    call %s\n", node->value);
        if (needs_pad) {
            fprintf(cg->output, "    add rsp, 8\n");
        }
        /* The call returns its value in rax; type it with the callee's
         * declared return type so surrounding expressions promote and
         * combine with the correct common type. Stage 12-05: when the
         * callee returns a pointer, attach its full Type chain so
         * downstream pointer checks (return statement, declaration
         * init) see the correct pointer type. */
        node->result_type = node->decl_type;
        if (callee && callee->decl_type == TYPE_POINTER) {
            node->full_type = callee->full_type;
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
                fprintf(stderr,
                        "error: operator '%s' not supported on pointer operands\n",
                        bw);
                exit(1);
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
            return;
        }
        if (strcmp(node->value, "<<") == 0 || strcmp(node->value, ">>") == 0) {
            const char *sop = node->value;
            TypeKind lt = expr_result_type(cg, node->children[0]);
            TypeKind rt = expr_result_type(cg, node->children[1]);
            if (lt == TYPE_POINTER || rt == TYPE_POINTER) {
                fprintf(stderr,
                        "error: operator '%s' not supported on pointer operands\n",
                        sop);
                exit(1);
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
            const char *insn = (strcmp(sop, "<<") == 0) ? "shl" : "sar";
            if (result == TYPE_LONG) {
                fprintf(cg->output, "    %s rax, cl\n", insn);
            } else {
                fprintf(cg->output, "    %s eax, cl\n", insn);
            }
            node->result_type = result;
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
        if (is_arith || is_cmp) {
            TypeKind lt = expr_result_type(cg, node->children[0]);
            TypeKind rt = expr_result_type(cg, node->children[1]);
            if (lt == TYPE_POINTER || rt == TYPE_POINTER) {
                if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
                    is_pointer_cmp = 1;
                    common = TYPE_LONG;
                } else if (strcmp(op, "+") == 0) {
                    if (lt == TYPE_POINTER && rt == TYPE_POINTER) {
                        fprintf(stderr,
                                "error: cannot add two pointers\n");
                        exit(1);
                    }
                    is_pointer_arith = 1;
                    common = TYPE_LONG;
                } else if (strcmp(op, "-") == 0) {
                    if (lt == TYPE_POINTER && rt == TYPE_POINTER) {
                        fprintf(stderr,
                                "error: pointer subtraction not supported\n");
                        exit(1);
                    }
                    if (rt == TYPE_POINTER) {
                        fprintf(stderr,
                                "error: cannot subtract pointer from integer\n");
                        exit(1);
                    }
                    is_pointer_arith = 1;
                    common = TYPE_LONG;
                } else {
                    fprintf(stderr,
                            "error: operator '%s' not supported on pointer operands\n",
                            op);
                    exit(1);
                }
            } else {
                common = common_arith_kind(lt, rt);
            }
        }

        /* Evaluate left into rax/eax */
        codegen_expression(cg, node->children[0]);
        if ((is_arith || is_cmp) && common == TYPE_LONG &&
            node->children[0]->result_type != TYPE_LONG &&
            node->children[0]->result_type != TYPE_POINTER) {
            fprintf(cg->output, "    movsxd rax, eax\n");
        }
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;
        /* Evaluate right into rax/eax */
        codegen_expression(cg, node->children[1]);
        if ((is_arith || is_cmp) && common == TYPE_LONG &&
            node->children[1]->result_type != TYPE_LONG &&
            node->children[1]->result_type != TYPE_POINTER) {
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
                    fprintf(stderr,
                            "error: incompatible pointer types in comparison\n");
                    exit(1);
                }
            } else if (lhs_ptr && !rhs_ptr) {
                if (!is_null_pointer_constant(rhs)) {
                    fprintf(stderr,
                            "error: comparing pointer with non zero integer\n");
                    exit(1);
                }
            } else if (!lhs_ptr && rhs_ptr) {
                if (!is_null_pointer_constant(lhs)) {
                    fprintf(stderr,
                            "error: comparing pointer with non zero integer\n");
                    exit(1);
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
                fprintf(stderr,
                        "error: pointer arithmetic missing pointer type\n");
                exit(1);
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
                fprintf(cg->output, "    cqo\n");
                fprintf(cg->output, "    idiv rcx\n");
            } else { /* "%" */
                /* left % right: remainder of rcx / rax. Place dividend
                 * in rax and divisor in rcx, sign-extend into rdx, then
                 * read the remainder out of rdx. */
                fprintf(cg->output, "    xchg rax, rcx\n");
                fprintf(cg->output, "    cqo\n");
                fprintf(cg->output, "    idiv rcx\n");
                fprintf(cg->output, "    mov rax, rdx\n");
            }
            node->result_type = TYPE_LONG;
            return;
        }
        if (strcmp(op, "+") == 0) {
            fprintf(cg->output, "    add eax, ecx\n");
            node->result_type = TYPE_INT;
        } else if (strcmp(op, "-") == 0) {
            /* left - right: ecx - eax */
            fprintf(cg->output, "    sub ecx, eax\n");
            fprintf(cg->output, "    mov eax, ecx\n");
            node->result_type = TYPE_INT;
        } else if (strcmp(op, "*") == 0) {
            fprintf(cg->output, "    imul eax, ecx\n");
            node->result_type = TYPE_INT;
        } else if (strcmp(op, "/") == 0) {
            /* left / right: ecx / eax */
            fprintf(cg->output, "    xchg eax, ecx\n");
            fprintf(cg->output, "    cdq\n");
            fprintf(cg->output, "    idiv ecx\n");
            node->result_type = TYPE_INT;
        } else if (strcmp(op, "%") == 0) {
            /* left % right: remainder of ecx / eax. After xchg the
             * dividend is in eax and the divisor in ecx; cdq sign-
             * extends eax into edx:eax and idiv leaves the remainder
             * in edx. */
            fprintf(cg->output, "    xchg eax, ecx\n");
            fprintf(cg->output, "    cdq\n");
            fprintf(cg->output, "    idiv ecx\n");
            fprintf(cg->output, "    mov eax, edx\n");
            node->result_type = TYPE_INT;
        } else {
            /* Comparisons: compare left (rcx/ecx) with right (rax/eax),
             * using the width of the common type after promotion. Result
             * is a normalized 0/1 in eax of type int. */
            const char *setcc = NULL;
            if      (strcmp(op, "==") == 0) setcc = "sete";
            else if (strcmp(op, "!=") == 0) setcc = "setne";
            else if (strcmp(op, "<")  == 0) setcc = "setl";
            else if (strcmp(op, "<=") == 0) setcc = "setle";
            else if (strcmp(op, ">")  == 0) setcc = "setg";
            else if (strcmp(op, ">=") == 0) setcc = "setge";
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

static void codegen_statement(CodeGen *cg, ASTNode *node, int is_main) {
    if (node->type == AST_DECLARATION) {
        /* Duplicate check limited to the current scope only — shadowing is allowed. */
        for (int i = cg->scope_start; i < cg->local_count; i++) {
            if (strcmp(cg->locals[i].name, node->value) == 0) {
                fprintf(stderr, "error: duplicate declaration of variable '%s'\n", node->value);
                exit(1);
            }
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
        if (node->decl_type == TYPE_ARRAY) {
            int size = node->full_type->size;
            int align = node->full_type->base->alignment;
            int offset = codegen_add_var(cg, node->value, size, align,
                                         node->decl_type, node->full_type);
            if (node->child_count > 0) {
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
                    fprintf(stderr,
                            "error: variable '%s' assigning pointer to non pointer\n",
                            node->value);
                    exit(1);
                }
                if (init_kind != TYPE_POINTER) {
                    fprintf(stderr,
                            "error: variable '%s' assigning non pointer to pointer\n",
                            node->value);
                    exit(1);
                }
                if (!pointer_types_equal(node->children[0]->full_type,
                                         node->full_type)) {
                    fprintf(stderr,
                            "error: variable '%s' incompatible pointer type in initializer\n",
                            node->value);
                    exit(1);
                }
            }
            /* Pointer values live in the full rax already (lea / 8-byte
             * load), so they take the same store path as long values
             * without the movsxd widening step. A null pointer constant
             * also takes the 8-byte path so the full slot is written. */
            int rhs_is_long = (init_kind == TYPE_LONG ||
                               init_kind == TYPE_POINTER ||
                               rhs_is_null_ptr);
            emit_store_local(cg, offset, size, rhs_is_long);
        }
    } else if (node->type == AST_RETURN_STATEMENT) {
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
                fprintf(stderr,
                        "error: function '%s' returning pointer from non pointer function\n",
                        cg->current_func);
                exit(1);
            }
            if (src_kind != TYPE_POINTER) {
                fprintf(stderr,
                        "error: function '%s' returning non pointer; expected pointer\n",
                        cg->current_func);
                exit(1);
            }
            if (!pointer_types_equal(node->children[0]->full_type,
                                     cg->current_return_full_type)) {
                fprintf(stderr,
                        "error: function '%s' returning incorrect pointer type\n",
                        cg->current_func);
                exit(1);
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
        if (node->children[0]) {
            codegen_expression(cg, node->children[0]);
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
            fprintf(stderr, "error: switch nesting exceeds max depth %d\n",
                    MAX_SWITCH_DEPTH);
            exit(1);
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
            fprintf(stderr, "error: undefined label '%s' in function '%s'\n",
                    node->value, cg->current_func);
            exit(1);
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
        if (num_params > 6) {
            fprintf(stderr,
                    "error: function '%s' has %d parameters; max supported is 6\n",
                    node->value, num_params);
            exit(1);
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
         * Rounded up to a 16-byte multiple. */
        int stack_size = num_params * 8 + compute_decl_bytes(body);
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
        fprintf(cg->output, "global %s\n", node->value);
        fprintf(cg->output, "%s:\n", node->value);
        cg->has_frame = 1;
        fprintf(cg->output, "    push rbp\n");
        fprintf(cg->output, "    mov rbp, rsp\n");
        if (stack_size > 0) {
            fprintf(cg->output, "    sub rsp, %d\n", stack_size);
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
        for (int i = 0; i < num_params; i++) {
            TypeKind pt = node->children[i]->decl_type;
            int sz = type_kind_bytes(pt);
            int offset = codegen_add_var(cg, node->children[i]->value, sz, sz,
                                         pt, node->children[i]->full_type);
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

        /* Generate body statements directly — the function body acts as the outermost scope. */
        for (int i = 0; i < body->child_count; i++) {
            codegen_statement(cg, body->children[i], is_main);
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

void codegen_translation_unit(CodeGen *cg, ASTNode *node) {
    cg->tu_root = node;
    fprintf(cg->output, "section .text\n");
    if (node->type == AST_TRANSLATION_UNIT) {
        codegen_emit_externs(cg, node);
        for (int i = 0; i < node->child_count; i++) {
            codegen_function(cg, node->children[i]);
        }
    }
    codegen_emit_string_pool(cg);
}
