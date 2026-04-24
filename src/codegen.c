#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "type.h"

static int type_kind_bytes(TypeKind kind) {
    switch (kind) {
    case TYPE_CHAR:  return 1;
    case TYPE_SHORT: return 2;
    case TYPE_INT:   return 4;
    case TYPE_LONG:  return 8;
    }
    return 4;
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
 * Allocate a local of `size` bytes. Stack grows down from rbp, so the
 * variable's rbp-relative offset is advanced by `size` and then aligned
 * up to a multiple of `size` (natural alignment for our integer types).
 */
static int codegen_add_var(CodeGen *cg, const char *name, int size) {
    cg->stack_offset += size;
    cg->stack_offset = (cg->stack_offset + size - 1) & ~(size - 1);
    strncpy(cg->locals[cg->local_count].name, name, 255);
    cg->locals[cg->local_count].name[255] = '\0';
    cg->locals[cg->local_count].offset = cg->stack_offset;
    cg->locals[cg->local_count].size = size;
    cg->local_count++;
    return cg->stack_offset;
}

/*
 * Conservative upper bound on stack bytes needed for locals: 8 bytes
 * per declaration (largest supported integer type plus worst-case
 * alignment padding). The prologue rounds this up to 16.
 */
static int compute_decl_bytes(ASTNode *node) {
    if (!node) return 0;
    int total = (node->type == AST_DECLARATION) ? 8 : 0;
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
        t = TYPE_INT;
        break;
    case AST_VAR_REF: {
        LocalVar *lv = codegen_find_var(cg, node->value);
        t = lv ? promote_kind(type_kind_from_size(lv->size)) : TYPE_INT;
        break;
    }
    case AST_UNARY_OP:
        if (strcmp(node->value, "+") == 0 || strcmp(node->value, "-") == 0) {
            t = promote_kind(expr_result_type(cg, node->children[0]));
        } else {
            t = TYPE_INT; /* ! stays 32-bit */
        }
        break;
    case AST_BINARY_OP: {
        const char *op = node->value;
        if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
            strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
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
    default:
        t = TYPE_INT;
        break;
    }
    node->result_type = t;
    return t;
}

static void codegen_expression(CodeGen *cg, ASTNode *node) {
    if (node->type == AST_INT_LITERAL) {
        node->result_type = TYPE_INT;
        fprintf(cg->output, "    mov eax, %s\n", node->value);
        return;
    }
    if (node->type == AST_VAR_REF) {
        LocalVar *lv = codegen_find_var(cg, node->value);
        if (!lv) {
            fprintf(stderr, "error: undeclared variable '%s'\n", node->value);
            exit(1);
        }
        node->result_type = promote_kind(type_kind_from_size(lv->size));
        emit_load_local(cg, lv->offset, lv->size);
        return;
    }
    if (node->type == AST_ASSIGNMENT) {
        LocalVar *lv = codegen_find_var(cg, node->value);
        if (!lv) {
            fprintf(stderr, "error: undeclared variable '%s'\n", node->value);
            exit(1);
        }
        codegen_expression(cg, node->children[0]);
        int rhs_is_long = (node->children[0]->result_type == TYPE_LONG);
        emit_store_local(cg, lv->offset, lv->size, rhs_is_long);
        node->result_type = type_kind_from_size(lv->size);
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
            if (node->children[0]->result_type == TYPE_LONG) {
                fprintf(cg->output, "    cmp rax, 0\n");
            } else {
                fprintf(cg->output, "    cmp eax, 0\n");
            }
            fprintf(cg->output, "    sete al\n");
            fprintf(cg->output, "    movzx eax, al\n");
            node->result_type = TYPE_INT;
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
    if (node->type == AST_FUNCTION_CALL) {
        static const char *arg_regs[6] = {
            "rdi", "rsi", "rdx", "rcx", "r8", "r9"
        };
        int nargs = node->child_count;
        /* Evaluate arguments left-to-right, pushing each result. */
        for (int i = 0; i < nargs; i++) {
            codegen_expression(cg, node->children[i]);
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
         * combine with the correct common type. */
        node->result_type = node->decl_type;
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
        const char *op = node->value;
        int is_arith = (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
                        strcmp(op, "*") == 0 || strcmp(op, "/") == 0);
        int is_cmp = (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
                      strcmp(op, "<")  == 0 || strcmp(op, "<=") == 0 ||
                      strcmp(op, ">")  == 0 || strcmp(op, ">=") == 0);
        /* For arithmetic and comparison operators, select a common type
         * after promotion. If the common type is long, both operands must
         * live in the full rax before the op — sign-extend int-sized
         * sides with movsxd. */
        TypeKind common = TYPE_INT;
        if (is_arith || is_cmp) {
            TypeKind lt = expr_result_type(cg, node->children[0]);
            TypeKind rt = expr_result_type(cg, node->children[1]);
            common = common_arith_kind(lt, rt);
        }

        /* Evaluate left into rax/eax */
        codegen_expression(cg, node->children[0]);
        if ((is_arith || is_cmp) && common == TYPE_LONG &&
            node->children[0]->result_type != TYPE_LONG) {
            fprintf(cg->output, "    movsxd rax, eax\n");
        }
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;
        /* Evaluate right into rax/eax */
        codegen_expression(cg, node->children[1]);
        if ((is_arith || is_cmp) && common == TYPE_LONG &&
            node->children[1]->result_type != TYPE_LONG) {
            fprintf(cg->output, "    movsxd rax, eax\n");
        }
        /* Pop left into rcx; now rcx=left, rax=right */
        fprintf(cg->output, "    pop rcx\n");
        cg->push_depth--;

        if (is_arith && common == TYPE_LONG) {
            if (strcmp(op, "+") == 0) {
                fprintf(cg->output, "    add rax, rcx\n");
            } else if (strcmp(op, "-") == 0) {
                /* left - right: rcx - rax */
                fprintf(cg->output, "    sub rcx, rax\n");
                fprintf(cg->output, "    mov rax, rcx\n");
            } else if (strcmp(op, "*") == 0) {
                fprintf(cg->output, "    imul rax, rcx\n");
            } else { /* "/" */
                /* left / right: rcx / rax */
                fprintf(cg->output, "    xchg rax, rcx\n");
                fprintf(cg->output, "    cqo\n");
                fprintf(cg->output, "    idiv rcx\n");
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
        int size = type_kind_bytes(node->decl_type);
        int offset = codegen_add_var(cg, node->value, size);
        if (node->child_count > 0) {
            codegen_expression(cg, node->children[0]);
            int rhs_is_long = (node->children[0]->result_type == TYPE_LONG);
            emit_store_local(cg, offset, size, rhs_is_long);
        }
    } else if (node->type == AST_RETURN_STATEMENT) {
        codegen_expression(cg, node->children[0]);
        if (is_main) {
            fprintf(cg->output, "    mov edi, eax\n");
            fprintf(cg->output, "    mov eax, 60\n");
            fprintf(cg->output, "    syscall\n");
        } else {
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

        /* Pre-walk the body to collect user labels; rejects duplicates. */
        collect_user_labels(cg, body);

        /* Compute stack space: 4 bytes per (int-only) parameter plus a
         * conservative 8-byte upper bound per body declaration. Rounded
         * up to a 16-byte multiple. */
        int stack_size = num_params * 4 + compute_decl_bytes(body);
        if (stack_size % 16 != 0)
            stack_size = (stack_size + 15) & ~15;

        /* Function label and prologue */
        fprintf(cg->output, "global %s\n", node->value);
        fprintf(cg->output, "%s:\n", node->value);
        cg->has_frame = (stack_size > 0);
        if (cg->has_frame) {
            fprintf(cg->output, "    push rbp\n");
            fprintf(cg->output, "    mov rbp, rsp\n");
            fprintf(cg->output, "    sub rsp, %d\n", stack_size);
        }

        /*
         * Parameters share the outermost body scope (scope_start stays at 0).
         * A body-level declaration that collides with a parameter name is
         * rejected by the existing duplicate-detection check in
         * codegen_statement for AST_DECLARATION.
         */
        static const char *param_regs[6] = {
            "edi", "esi", "edx", "ecx", "r8d", "r9d"
        };
        for (int i = 0; i < num_params; i++) {
            int offset = codegen_add_var(cg, node->children[i]->value, 4);
            fprintf(cg->output, "    mov [rbp - %d], %s\n",
                    offset, param_regs[i]);
        }

        /* Generate body statements directly — the function body acts as the outermost scope. */
        for (int i = 0; i < body->child_count; i++) {
            codegen_statement(cg, body->children[i], is_main);
        }
    }
}

void codegen_translation_unit(CodeGen *cg, ASTNode *node) {
    fprintf(cg->output, "section .text\n");
    if (node->type == AST_TRANSLATION_UNIT) {
        for (int i = 0; i < node->child_count; i++) {
            codegen_function(cg, node->children[i]);
        }
    }
}
