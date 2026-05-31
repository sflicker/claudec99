#ifndef CCOMPILER_CODEGEN_H
#define CCOMPILER_CODEGEN_H

#include <stdio.h>
#include "ast.h"
#include "type.h"

typedef struct {
    char name[MAX_NAME_LEN];
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
    char static_label[MAX_NAME_LEN];
} LocalVar;

/* Stage 71: one entry per block-scope static variable declared during
 * function body emission. Accumulated during codegen_function and
 * emitted to .data or .bss after all function bodies. */
typedef struct {
    char label[MAX_NAME_LEN];
    TypeKind kind;
    Type *full_type;
    int size;
    int is_initialized;
    long init_value;
    int is_unsigned;
} LocalStaticVar;

/* Stage 22-01: file-scope (global) variable. Accessed via RIP-relative
 * addressing ([rel name]) instead of [rbp - offset]. size is the byte
 * width for load/store; full_type is non-NULL for pointer and array
 * kinds and carries the complete type chain.
 * Stage 22-02: is_initialized set for globals with a constant initializer;
 * init_value holds that value. Initialized globals go to .data; others to .bss. */
typedef struct {
    char name[MAX_NAME_LEN];
    int size;
    TypeKind kind;
    Type *full_type;
    int is_initialized;
    long init_value;
    /* Stage 25-02: when is_label_init is set, the global is initialized to
     * the address of a named symbol (e.g. a function pointer initialized
     * from a function designator). init_label holds the symbol name. */
    int is_label_init;
    char init_label[MAX_NAME_LEN];
    /* Stage 39: set when the variable itself is const-qualified. */
    int is_const;
    /* Stage 40: set when the variable has an unsigned integer type. */
    int is_unsigned;
    /* Stage 43: non-NULL when the initializer is a string literal or
     * initializer-list (array/pointer-array cases). Points into the AST,
     * which outlives codegen_translate_unit. */
    struct ASTNode *init_node;
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
typedef struct {
    ASTNode *nodes[MAX_SWITCH_LABELS];
    int labels[MAX_SWITCH_LABELS];
    int count;
    int default_label;
} SwitchCtx;

typedef struct {
    FILE *output;
    int label_count;
    LocalVar locals[MAX_LOCALS];
    int local_count;
    /* Stage 22-01: file-scope globals, registered before function codegen. */
    GlobalVar globals[MAX_GLOBALS];
    int global_count;
    int stack_offset;
    int scope_start;
    int push_depth;
    int has_frame;
    BreakFrame break_stack[MAX_BREAK_DEPTH];
    int break_depth;
    SwitchCtx switch_stack[MAX_SWITCH_DEPTH];
    int switch_depth;
    /* Per-function user label table (populated by a pre-walk before
     * body emission; used to reject duplicates and missing goto
     * targets). Assembly names are prefixed by `current_func` so
     * reused label names in different functions never collide. */
    char user_labels[MAX_USER_LABELS][MAX_NAME_LEN];
    int user_label_count;
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
     * is appended here and assigned the index `string_pool_count`
     * (used as the suffix in `Lstr<N>`). After all functions are
     * emitted, the pool is walked once to write the `.rodata`
     * section. Storing AST node pointers is safe because the AST
     * outlives `codegen_translation_unit`. */
    ASTNode *string_pool[MAX_STRING_LITERALS];
    int string_pool_count;
    /* Stage 66: when set, warnings are promoted to errors (exit 1). */
    int warnings_are_errors;
    /* Stage 71: block-scope static variable pool — accumulated across all
     * function bodies and emitted to .data/.bss after function code. */
    LocalStaticVar local_statics[MAX_LOCAL_STATICS];
    int local_static_count;
    /* Stage 75-04: variadic function register save area.
     * variadic_reg_save_offset: rbp-relative offset of the 304-byte register
     * save area (0 for non-variadic functions).
     * variadic_named_gp_params: min(named_param_count, 6) — used by va_start
     * to initialize gp_offset.
     * variadic_named_stack_params: max(0, named_param_count - 6) — used by
     * va_start to compute overflow_arg_area. */
    int variadic_reg_save_offset;
    int variadic_named_gp_params;
    int variadic_named_stack_params;
} CodeGen;

void codegen_init(CodeGen *cg, FILE *output);
void codegen_translation_unit(CodeGen *cg, ASTNode *node);

#endif
