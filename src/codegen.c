#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

void codegen_init(CodeGen *cg, FILE *output) {
    cg->output = output;
    cg->label_count = 0;
    cg->local_count = 0;
    cg->stack_offset = 0;
    cg->scope_start = 0;
    cg->push_depth = 0;
    cg->has_frame = 0;
    cg->loop_depth = 0;
}

static int codegen_find_var(CodeGen *cg, const char *name) {
    /* Walk backward so the innermost (most recently declared) shadow wins. */
    for (int i = cg->local_count - 1; i >= 0; i--) {
        if (strcmp(cg->locals[i].name, name) == 0)
            return cg->locals[i].offset;
    }
    return 0; /* not found (valid offsets start at 4) */
}

static int codegen_add_var(CodeGen *cg, const char *name) {
    cg->stack_offset += 4;
    strncpy(cg->locals[cg->local_count].name, name, 255);
    cg->locals[cg->local_count].name[255] = '\0';
    cg->locals[cg->local_count].offset = cg->stack_offset;
    cg->local_count++;
    return cg->stack_offset;
}

static int count_declarations(ASTNode *node) {
    if (!node) return 0;
    int count = (node->type == AST_DECLARATION) ? 1 : 0;
    for (int i = 0; i < node->child_count; i++) {
        count += count_declarations(node->children[i]);
    }
    return count;
}

static void codegen_expression(CodeGen *cg, ASTNode *node) {
    if (node->type == AST_INT_LITERAL) {
        fprintf(cg->output, "    mov eax, %s\n", node->value);
        return;
    }
    if (node->type == AST_VAR_REF) {
        int offset = codegen_find_var(cg, node->value);
        if (offset == 0) {
            fprintf(stderr, "error: undeclared variable '%s'\n", node->value);
            exit(1);
        }
        fprintf(cg->output, "    mov eax, [rbp - %d]\n", offset);
        return;
    }
    if (node->type == AST_ASSIGNMENT) {
        int offset = codegen_find_var(cg, node->value);
        if (offset == 0) {
            fprintf(stderr, "error: undeclared variable '%s'\n", node->value);
            exit(1);
        }
        codegen_expression(cg, node->children[0]);
        fprintf(cg->output, "    mov [rbp - %d], eax\n", offset);
        return;
    }
    if (node->type == AST_UNARY_OP) {
        codegen_expression(cg, node->children[0]);
        const char *op = node->value;
        if (strcmp(op, "-") == 0) {
            fprintf(cg->output, "    neg eax\n");
        } else if (strcmp(op, "!") == 0) {
            fprintf(cg->output, "    cmp eax, 0\n");
            fprintf(cg->output, "    sete al\n");
            fprintf(cg->output, "    movzx eax, al\n");
        }
        /* unary + is a no-op */
        return;
    }
    if (node->type == AST_PREFIX_INC_DEC) {
        /* ++x or --x: update variable, result is new value */
        const char *var_name = node->children[0]->value;
        int offset = codegen_find_var(cg, var_name);
        if (offset == 0) {
            fprintf(stderr, "error: undeclared variable '%s'\n", var_name);
            exit(1);
        }
        fprintf(cg->output, "    mov eax, [rbp - %d]\n", offset);
        if (strcmp(node->value, "++") == 0) {
            fprintf(cg->output, "    add eax, 1\n");
        } else {
            fprintf(cg->output, "    sub eax, 1\n");
        }
        fprintf(cg->output, "    mov [rbp - %d], eax\n", offset);
        return;
    }
    if (node->type == AST_POSTFIX_INC_DEC) {
        /* x++ or x--: result is old value, then update variable */
        const char *var_name = node->children[0]->value;
        int offset = codegen_find_var(cg, var_name);
        if (offset == 0) {
            fprintf(stderr, "error: undeclared variable '%s'\n", var_name);
            exit(1);
        }
        fprintf(cg->output, "    mov eax, [rbp - %d]\n", offset);
        fprintf(cg->output, "    mov ecx, eax\n");  /* save old value */
        if (strcmp(node->value, "++") == 0) {
            fprintf(cg->output, "    add ecx, 1\n");
        } else {
            fprintf(cg->output, "    sub ecx, 1\n");
        }
        fprintf(cg->output, "    mov [rbp - %d], ecx\n", offset);
        /* eax still holds the old value */
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
        return;
    }
    if (node->type == AST_BINARY_OP) {
        const char *bop = node->value;
        /* Short-circuit logical operators — do not evaluate RHS unconditionally */
        if (strcmp(bop, "&&") == 0) {
            int label_id = cg->label_count++;
            codegen_expression(cg, node->children[0]);
            fprintf(cg->output, "    cmp eax, 0\n");
            fprintf(cg->output, "    je .L_and_false_%d\n", label_id);
            codegen_expression(cg, node->children[1]);
            fprintf(cg->output, "    cmp eax, 0\n");
            fprintf(cg->output, "    setne al\n");
            fprintf(cg->output, "    movzx eax, al\n");
            fprintf(cg->output, "    jmp .L_and_end_%d\n", label_id);
            fprintf(cg->output, ".L_and_false_%d:\n", label_id);
            fprintf(cg->output, "    mov eax, 0\n");
            fprintf(cg->output, ".L_and_end_%d:\n", label_id);
            return;
        }
        if (strcmp(bop, "||") == 0) {
            int label_id = cg->label_count++;
            codegen_expression(cg, node->children[0]);
            fprintf(cg->output, "    cmp eax, 0\n");
            fprintf(cg->output, "    jne .L_or_true_%d\n", label_id);
            codegen_expression(cg, node->children[1]);
            fprintf(cg->output, "    cmp eax, 0\n");
            fprintf(cg->output, "    setne al\n");
            fprintf(cg->output, "    movzx eax, al\n");
            fprintf(cg->output, "    jmp .L_or_end_%d\n", label_id);
            fprintf(cg->output, ".L_or_true_%d:\n", label_id);
            fprintf(cg->output, "    mov eax, 1\n");
            fprintf(cg->output, ".L_or_end_%d:\n", label_id);
            return;
        }
        /* Evaluate left into eax, push it */
        codegen_expression(cg, node->children[0]);
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;
        /* Evaluate right into eax */
        codegen_expression(cg, node->children[1]);
        /* Pop left into ecx; now ecx=left, eax=right */
        fprintf(cg->output, "    pop rcx\n");
        cg->push_depth--;
        const char *op = node->value;
        if (strcmp(op, "+") == 0) {
            fprintf(cg->output, "    add eax, ecx\n");
        } else if (strcmp(op, "-") == 0) {
            /* left - right: ecx - eax */
            fprintf(cg->output, "    sub ecx, eax\n");
            fprintf(cg->output, "    mov eax, ecx\n");
        } else if (strcmp(op, "*") == 0) {
            fprintf(cg->output, "    imul eax, ecx\n");
        } else if (strcmp(op, "/") == 0) {
            /* left / right: ecx / eax */
            fprintf(cg->output, "    xchg eax, ecx\n");
            fprintf(cg->output, "    cdq\n");
            fprintf(cg->output, "    idiv ecx\n");
        } else {
            /* Comparisons: compare ecx (left) with eax (right), set al, zero-extend */
            const char *setcc = NULL;
            if      (strcmp(op, "==") == 0) setcc = "sete";
            else if (strcmp(op, "!=") == 0) setcc = "setne";
            else if (strcmp(op, "<")  == 0) setcc = "setl";
            else if (strcmp(op, "<=") == 0) setcc = "setle";
            else if (strcmp(op, ">")  == 0) setcc = "setg";
            else if (strcmp(op, ">=") == 0) setcc = "setge";
            fprintf(cg->output, "    cmp ecx, eax\n");
            fprintf(cg->output, "    %s al\n", setcc);
            fprintf(cg->output, "    movzx eax, al\n");
        }
        return;
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
        int offset = codegen_add_var(cg, node->value);
        if (node->child_count > 0) {
            codegen_expression(cg, node->children[0]);
            fprintf(cg->output, "    mov [rbp - %d], eax\n", offset);
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
        fprintf(cg->output, "    cmp eax, 0\n");
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
        cg->loop_stack[cg->loop_depth].break_label = label_id;
        cg->loop_stack[cg->loop_depth].continue_label = label_id;
        cg->loop_depth++;
        fprintf(cg->output, ".L_while_start_%d:\n", label_id);
        fprintf(cg->output, ".L_continue_%d:\n", label_id);
        codegen_expression(cg, node->children[0]);
        fprintf(cg->output, "    cmp eax, 0\n");
        fprintf(cg->output, "    je .L_while_end_%d\n", label_id);
        codegen_statement(cg, node->children[1], is_main);
        fprintf(cg->output, "    jmp .L_while_start_%d\n", label_id);
        fprintf(cg->output, ".L_while_end_%d:\n", label_id);
        fprintf(cg->output, ".L_break_%d:\n", label_id);
        cg->loop_depth--;
    } else if (node->type == AST_FOR_STATEMENT) {
        /* children: [0]=init, [1]=condition, [2]=update, [3]=body (any may be NULL except body) */
        int label_id = cg->label_count++;
        cg->loop_stack[cg->loop_depth].break_label = label_id;
        cg->loop_stack[cg->loop_depth].continue_label = label_id;
        cg->loop_depth++;
        if (node->children[0]) {
            codegen_expression(cg, node->children[0]);
        }
        fprintf(cg->output, ".L_for_start_%d:\n", label_id);
        if (node->children[1]) {
            codegen_expression(cg, node->children[1]);
            fprintf(cg->output, "    cmp eax, 0\n");
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
        cg->loop_depth--;
    } else if (node->type == AST_BREAK_STATEMENT) {
        int id = cg->loop_stack[cg->loop_depth - 1].break_label;
        fprintf(cg->output, "    jmp .L_break_%d\n", id);
    } else if (node->type == AST_CONTINUE_STATEMENT) {
        int id = cg->loop_stack[cg->loop_depth - 1].continue_label;
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

        /* Compute stack space: one slot per parameter plus each declaration in the body. */
        int num_vars = count_declarations(body);
        int total_slots = num_params + num_vars;
        int stack_size = total_slots * 4;
        if (stack_size % 16 != 0)
            stack_size = (stack_size + 15) & ~15;

        /* Function label and prologue */
        fprintf(cg->output, "global %s\n", node->value);
        fprintf(cg->output, "%s:\n", node->value);
        cg->has_frame = (total_slots > 0);
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
            int offset = codegen_add_var(cg, node->children[i]->value);
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
