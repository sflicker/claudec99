#ifndef CCOMPILER_CODEGEN_H
#define CCOMPILER_CODEGEN_H

#include <stdio.h>
#include "ast.h"
#include "type.h"
#include "vec.h"

typedef struct {
    const char *name;
    int offset;
    int size;
    /* Stage 12-02: declared kind so codegen can distinguish a pointer
     * local from a long. `full_type` is set only when kind ==
     * TYPE_POINTER and carries the pointer chain head — needed by
     * dereference codegen to pick the load width and by address-of
     * to construct a `pointer-to-T` result type. */
    TypeKind kind;
    Type *full_type;
    /* Stage 39: set when the variable itself is const-qualified. */
    int is_const;
    /* Stage 40: set when the variable has an unsigned integer type. */
    int is_unsigned;
    /* Stage 71: set for block-scope static variables. When is_static is
     * set, storage is in a named static symbol addressed via [rel label]
     * instead of [rbp - offset]; offset is unused. */
    int is_static;
    /* Stage 95-11: pointer into util_strdup'd storage for the generated label. */
    const char *static_label;
} LocalVar;

/* Stage 71: one entry per block-scope static variable declared during
 * function body emission. Accumulated during codegen_function and
 * emitted to .data or .bss after all function bodies. */
typedef struct {
    /* Stage 95-11: pointer into util_strdup'd storage for the generated label. */
    const char *label;
    TypeKind kind;
    Type *full_type;
    int size;
    int is_initialized;
    long init_value;
    int is_unsigned;
    /* Stage 101: array/struct brace-list or string-literal initializer node;
     * NULL for scalars and for any uninitialized (.bss) entry. */
    struct ASTNode *init_node;
} LocalStaticVar;

/* Stage 22-01: file-scope (global) variable. Accessed via RIP-relative
 * addressing ([rel name]) instead of [rbp - offset]. size is the byte
 * width for load/store; full_type is non-NULL for pointer and array
 * kinds and carries the complete type chain.
 * Stage 22-02: is_initialized set for globals with a constant initializer;
 * init_value holds that value. Initialized globals go to .data; others to .bss. */
typedef struct {
    /* Stage 95-11: pointer into lexer-owned storage (decl->value). */
    const char *name;
    int size;
    TypeKind kind;
    Type *full_type;
    int is_initialized;
    long init_value;
    /* Stage 25-02: when is_label_init is set, the global is initialized to
     * the address of a named symbol (e.g. a function pointer initialized
     * from a function designator). init_label holds the symbol name.
     * Stage 95-11: pointer into lexer-owned storage or util_strdup'd buffer. */
    int is_label_init;
    const char *init_label;
    /* Stage 39: set when the variable itself is const-qualified. */
    int is_const;
    /* Stage 40: set when the variable has an unsigned integer type. */
    int is_unsigned;
    /* Stage 43: non-NULL when the initializer is a string literal or
     * initializer-list (array/pointer-array cases). Points into the AST,
     * which outlives codegen_translate_unit. */
    struct ASTNode *init_node;
    /* Stage 84: set for extern-only object declarations (no storage here). */
    int is_extern;
    /* Stage 92: set for file-scope `static` variables (internal linkage — no
     * `global` NASM directive emitted). */
    int is_static_linkage;
} GlobalVar;

/* One entry per breakable construct (loop or switch). Switches set
 * continue_label to -1 since `continue` is not valid inside a switch
 * alone; AST_CONTINUE walks down to the nearest loop entry. */
typedef struct {
    int break_label;
    int continue_label;
} BreakFrame;

/* Per-switch context. Populated by a pre-walk of the switch body
 * that collects every case/default node pointer (stopping at nested
 * switches) and assigns each a unique label id. During body emission
 * a case/default node is matched against this table to emit its
 * pre-assigned label. */
/* Stage 95-12: one case/default label entry. Pairing the node pointer with
 * its label id keeps the two aligned by construction. */
typedef struct {
    ASTNode *node;
    int label;
} SwitchLabel;

typedef struct {
    /* Stage 95-12: label table is now dynamic (was parallel fixed arrays
     * capped at MAX_SWITCH_LABELS). `count` is now `entries.len`. */
    Vec entries;  /* SwitchLabel */
    int default_label;
} SwitchCtx;

typedef struct {
    FILE *output;
    int label_count;
    /* Stage 95-05: local variable table is now dynamic. */
    Vec locals;  /* LocalVar */
    /* Stage 22-01: file-scope globals, registered before function codegen.
     * Stage 95-05: dynamic. */
    Vec globals;  /* GlobalVar */
    int stack_offset;
    int scope_start;
    int push_depth;
    int has_frame;
    /* Stage 95-06: break stack is now dynamic. */
    Vec break_stack;  /* BreakFrame */
    int break_depth;
    /* Stage 95-07: switch stack is now dynamic. */
    Vec switch_stack;  /* SwitchCtx */
    int switch_depth;
    /* Per-function user label table (populated by a pre-walk before
     * body emission; used to reject duplicates and missing goto
     * targets). Assembly names are prefixed by `current_func` so
     * reused label names in different functions never collide.
     * Stage 95-11: converted from 2D char array to Vec of const char*. */
    Vec user_labels;  /* const char * */
    const char *current_func;
    /* Declared return type of the function currently being emitted —
     * used by AST_RETURN_STATEMENT to convert the return expression
     * to the function's declared return type. */
    TypeKind current_return_type;
    /* Stage 12-05: full Type chain for the current function's return
     * type. Non-NULL only when the function returns a pointer; carries
     * the head of the pointer chain so the return statement can do
     * strict chain matching against the return expression. */
    Type *current_return_full_type;
    /* Root of the translation unit being emitted; used to look up a
     * callee's AST_FUNCTION_DECL (and through it, the declared
     * parameter types) at each call site for argument conversion. */
    ASTNode *tu_root;
    /* Stage 14-03: per-translation-unit pool of string literals.
     * Each AST_STRING_LITERAL encountered during expression emission
     * is appended here and assigned the index == pool length before push
     * (used as the suffix in `Lstr<N>`). After all functions are
     * emitted, the pool is walked once to write the `.rodata`
     * section. Storing AST node pointers is safe because the AST
     * outlives `codegen_translation_unit`.
     * Stage 95-05: dynamic. */
    Vec string_pool;  /* ASTNode * */
    /* Stage 109: per-translation-unit pool of float/double literals.
     * Each unique raw text is assigned a Lfc<N> label and emitted to
     * .rodata as DD or DQ for NASM to encode as IEEE 754. */
    Vec fp_literals;  /* FpLiteral */
    /* Stage 110: track whether the sign-bit mask constants for FP unary
     * minus have been emitted to .rodata (Lfp_smask_f32 / Lfp_smask_f64).
     * Set on first use; the emit helper checks these before writing. */
    int fp_sign_mask_f32_emitted;
    int fp_sign_mask_f64_emitted;
    /* Stage 66: when set, warnings are promoted to errors (exit 1). */
    int warnings_are_errors;
    /* Stage 71: block-scope static variable pool — accumulated across all
     * function bodies and emitted to .data/.bss after function code.
     * Stage 95-04: converted from fixed array to Vec. */
    Vec local_statics;  /* LocalStaticVar */
    /* Stage 75-04: variadic function register save area.
     * variadic_reg_save_offset: rbp-relative offset of the 176-byte register
     * save area (0 for non-variadic functions).
     * variadic_named_gp_params: count of named GP params (min 6) — used by
     * va_start to initialize gp_offset.
     * variadic_named_xmm_params: count of named XMM (FP) params — used by
     * va_start to initialize fp_offset.
     * variadic_named_stack_params: max(0, named_gp_params - 6) — used by
     * va_start to compute overflow_arg_area. */
    int variadic_reg_save_offset;
    int variadic_named_gp_params;
    int variadic_named_xmm_params;
    int variadic_named_stack_params;
    /* Stage 91-01: struct-by-value return support.
     * current_sret_offset: rbp-relative offset of the slot holding the hidden
     * return pointer (rdi at entry) when the current function returns a
     * memory-class struct (>16 bytes); 0 otherwise.
     * struct_ret_scratch_base / struct_ret_scratch_cursor: a per-function
     * scratch region used to materialize struct-returning call results. The
     * base is the highest rbp offset of the region (reserved in the prologue);
     * the cursor bumps downward as each call site claims a temp. */
    int current_sret_offset;
    int struct_ret_scratch_base;
    int struct_ret_scratch_cursor;
    /* Stage 96: all strings codegen generated via codegen_intern() —
     * a single owned list so codegen_free() can release them all without
     * per-field ownership tracking. */
    Vec owned_strings;  /* char * */
} CodeGen;

void codegen_init(CodeGen *cg, FILE *output);
void codegen_translation_unit(CodeGen *cg, ASTNode *node);
/* Stage 96: release all heap storage owned by the code generator. */
void codegen_free(CodeGen *cg);

#endif
