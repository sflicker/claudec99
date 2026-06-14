# ClaudeC99 Stage 123 — ABI Bug Fixes: FP Stack Args, Indirect FP Calls, Address-Constant Initializers, FP Short-Circuit

## Goal

Fix five correctness bugs reported in `docs/defects/CC99_FIX_REPORT-02.md`:

| ID | Area | Symptom |
|----|------|---------|
| CC99-001 | SysV AMD64 ABI / non-variadic FP args | Stack-passed `double` args beyond `xmm7` are silently ignored |
| CC99-002 | SysV AMD64 ABI / variadic FP args | `va_arg(ap, double)` returns garbage for the 9th+ double argument |
| CC99-003 | Indirect calls / FP arguments | Function-pointer calls with `double` arguments produce wrong results |
| CC99-004 | Constant expressions / pointer initializers | Valid `&global`, `&global[N]`, and function-designator initializers are rejected |
| CC99-005 | FP logical operators | `&&` and `||` on `double` operands evaluate both sides (no short-circuit) |

This is a **codegen-and-parser stage**.  The tokenizer and AST are unchanged.

---

## CC99-001 & CC99-002 — Stack-Passed Floating-Point Arguments

### Root Cause

The SysV AMD64 ABI allocates `xmm0`–`xmm7` for the first 8 floating-point
arguments.  A 9th (or later) FP argument belongs to the "memory" class and is
placed in the stack-passed argument area.  On the caller side this means writing
it with `movss`/`movsd`, not `mov rax`.

`compute_call_layout()` builds an `ArgSlot` for each argument.  The `ArgSlot`
struct used two fields to describe where an argument lives:

- `xmm_idx >= 0` — argument goes in an XMM register.
- `xmm_idx < 0` — argument is memory-class (stack-passed).

The Phase 1 emission loop (which writes stack-passed arguments before the
`sub rsp` call frame is set up) tested `s->xmm_idx < 0` to decide between a
GP stack path (`mov [rsp + off], rax`) and an "else" path that was supposed to
handle FP stack arguments but was effectively dead code:

```c
} else if (s->xmm_idx >= 0) {
    /* was supposed to be FP stack, but xmm_idx is -1 for overflow FP too */
} else {
    /* GP stack — executed for ALL memory-class args, including overflow FP */
    fprintf(cg->output, "    mov [rsp + %d], rax\n", s->stack_off);
}
```

Both GP-stack args (`is_fp == 0`) and overflow-FP args (`is_fp == 1`) share
`xmm_idx == -1`, so the FP branch was unreachable.  Every stack-passed double
was stored as an integer register value, writing the wrong bits.

CC99-002 (variadic) is the same bug on the caller side: the variadic
`va_arg(ap, double)` mechanism reads from `overflow_arg_area` when the FP
register slots are exhausted, but the caller never wrote a valid IEEE-754
value there — it wrote `rax`.  The `va_arg` expansion then read back
floating-point garbage.

### Fix — Add `is_fp` to `ArgSlot`

**`src/codegen.c` — `ArgSlot` struct**:

```c
typedef struct {
    int is_struct;
    int is_fp;       /* argument is a floating-point type */
    int mem;
    int nbytes;
    int gp_start;
    int gp_count;
    int xmm_idx;
    int stack_off;
} ArgSlot;
```

**`compute_call_layout()`** — set the new field:

```c
s->is_fp = is_fp;
```

**Phase 1 direct-call emission** — check `is_fp` instead of `xmm_idx`:

```c
} else if (s->is_fp) {
    /* FP stack arg: overflow beyond xmm7. */
    codegen_expression(cg, node->children[i]);
    EMIT_ARG_CONVERT(node, callee, i);
    if (s->nbytes == 4)
        fprintf(cg->output, "    movss [rsp + %d], xmm0\n", s->stack_off);
    else
        fprintf(cg->output, "    movsd [rsp + %d], xmm0\n", s->stack_off);
} else {
    /* GP stack arg. */
    codegen_expression(cg, node->children[i]);
    EMIT_ARG_CONVERT(node, callee, i);
    fprintf(cg->output, "    mov [rsp + %d], rax\n", s->stack_off);
}
```

CC99-001 and CC99-002 share the same caller-side root cause.  Fixing Phase 1
of the direct-call path fixes both.

---

## CC99-003 — Indirect Calls With Floating-Point Arguments

### Root Cause

The `AST_INDIRECT_CALL` handler had two paths:

1. **N ≤ 6 scalar path** — pushed arguments left-to-right onto the stack, then
   moved them into GP registers (`rdi`, `rsi`, …).  XMM registers were never
   loaded.
2. **N > 6 scalar path** — similar GP-only logic with a stack spill region.

Neither path placed any argument into an XMM register.  A call like
`p(1.0, 2.0)` through a `double (*)(double, double)` function pointer would
load `rdi` = `1.0` bits and `rsi` = `2.0` bits, but the callee reads `xmm0`
and `xmm1`.  Only one double ended up in the right place (or none, depending
on argument count and register mapping).

### Fix — FP-Aware Path Using `CallLayout`

The indirect call handler now inspects the argument types before choosing an
emission strategy.  If any argument is a floating-point type the handler takes
a new path that mirrors the direct-call Phase 1 / Phase 2 logic:

**Detection**:
```c
int indirect_has_fp = 0;
for (int fi = 0; fi < nargs; fi++) {
    TypeKind pt = (fi < fn->param_count) ? fn->params[fi]->kind : TYPE_LONG;
    if (type_is_fp(pt)) { indirect_has_fp = 1; break; }
}
```

**Phase 1** — allocate a stack region and write memory-class args with
`movss`/`movsd` (FP) or `mov` (GP), using the same `ArgSlot` data computed by
`compute_call_layout()`.

**Phase 2** — spill register-class args to scratch slots in the region, then
restore each one into its target XMM or GP register after evaluation.

**Callee address**: the callee pointer is pushed onto the stack (`push rax`,
`push_depth++`) before the `sub rsp, region` allocation.  At call time the
callee is at `[rsp + region]`.  The call sequence is:
```asm
    mov r10, [rsp + region]
    call r10
    add rsp, region + 8
```

Alignment: the existing formula `((push_depth * 8 + region) % 16) != 0` already
accounts for the callee push because `cg->push_depth` is incremented before
`compute_call_layout` is called.

---

## CC99-004 — Address-Constant Initializers for Pointer Objects

### Root Cause

C99 §6.6p9 permits pointer objects with static storage duration to be
initialized with *address constants*: `&global`, `&global[N]`, and function
designators.  Two places in the compiler rejected these:

1. **Parser** (`src/parser.c` ~line 3823) — the global pointer initializer
   validation only permitted integer/char/string literals and `AST_VAR_REF`
   (bare function designators).  `AST_ADDR_OF` nodes (`&x`, `&xs[2]`) caused
   the `"non-constant initializer for global"` error.

2. **Codegen** — `codegen_add_global()` and `codegen_emit_data()` had no
   handling for `AST_ADDR_OF` init nodes.  `codegen_add_local_static()` called
   `eval_const_init()` on every pointer initializer; that function only handles
   integer constant expressions, so it rejected address constants.

### Fix — Parser

**`src/parser.c`** — extend the allowlist to include `AST_ADDR_OF`:

```c
if (init->type != AST_INT_LITERAL && init->type != AST_CHAR_LITERAL &&
    init->type != AST_STRING_LITERAL && init->type != AST_VAR_REF &&
    init->type != AST_ADDR_OF) {
    PARSER_ERROR(parser,
            "error: non-constant initializer for global '%s'\n", d.name);
}
```

### Fix — Global codegen

**`codegen_add_global()`** — store the `AST_ADDR_OF` init node instead of
calling `eval_const_init`:

```c
} else if (gv->kind == TYPE_POINTER && init->type == AST_ADDR_OF) {
    gv->init_node = init;
    gv->is_initialized = 1;
}
```

**`codegen_emit_data()`** — emit `dq label` or `dq label + offset` for the
two forms:

```c
} else if (gv->kind == TYPE_POINTER && gv->init_node &&
           gv->init_node->type == AST_ADDR_OF) {
    ASTNode *addr = gv->init_node;
    ASTNode *child = (addr->child_count > 0) ? addr->children[0] : NULL;
    if (child && child->type == AST_VAR_REF) {
        /* &global → dq global_name */
        fprintf(cg->output, "%s: dq %s\n", gv->name, child->value);
    } else if (child && child->type == AST_ARRAY_INDEX && ...) {
        /* &global[N] → dq global_name + N * elem_size */
        fprintf(cg->output, "%s: dq %s + %d\n",
                gv->name, arr_name, idx * elem_sz);
    }
}
```

### Fix — Block-scope static codegen

**`include/codegen.h`** — extend `LocalStaticVar` with two fields:

```c
typedef struct {
    const char *label;
    TypeKind kind;
    Type *full_type;
    int size;
    int is_initialized;
    long init_value;
    int is_unsigned;
    struct ASTNode *init_node;
    int is_label_init;      /* address-constant initializer flag */
    const char *init_label; /* symbol name (currently unused; init_node used instead) */
} LocalStaticVar;
```

**Block-scope scalar static in `codegen_add_local_static()`** — detect
address-constant initializers before `eval_const_init()`:

```c
int is_addr_const =
    (node->decl_type == TYPE_POINTER) &&
    (init->type == AST_VAR_REF || init->type == AST_ADDR_OF);
if (is_addr_const) {
    addr_init_node = init;
} else {
    init_value = eval_const_init(init, node->value);
}
/* ... */
new_sv.init_node = addr_init_node;
new_sv.is_label_init = (addr_init_node != NULL) ? 1 : 0;
new_sv.init_label = NULL;
```

All other `LocalStaticVar` construction sites (array-static, struct/union-static)
also initialize `is_label_init = 0; init_label = NULL;`.

**`codegen_emit_local_statics()`** — emit label-based data for `is_label_init`:

```c
} else if (sv->is_label_init && sv->init_node &&
           sv->init_node->type == AST_VAR_REF) {
    fprintf(cg->output, "%s: dq %s\n", sv->label, sv->init_node->value);
} else if (sv->is_label_init && sv->init_node &&
           sv->init_node->type == AST_ADDR_OF &&
           sv->init_node->child_count == 1) {
    ASTNode *child = sv->init_node->children[0];
    if (child->type == AST_VAR_REF) {
        fprintf(cg->output, "%s: dq %s\n", sv->label, child->value);
    } else if (child->type == AST_ARRAY_INDEX && ...) {
        fprintf(cg->output, "%s: dq %s + %d\n",
                sv->label, arr_name, idx * elem_sz);
    }
}
```

### Key Implementation Note: `AST_ADDR_OF` vs `AST_UNARY_OP`

The unary `&` operator produces `AST_ADDR_OF` nodes, **not**
`AST_UNARY_OP` nodes.  Any check using `AST_UNARY_OP` with
`strcmp(value, "&")` will silently fail to match `&x` expressions.  Always
use `AST_ADDR_OF` when testing for address-of expressions in the AST.

---

## CC99-005 — Floating-Point Logical Short-Circuit

### Root Cause

The `&&` and `||` handlers in `codegen_expression()` evaluated both operands
and performed a boolean combination, but for integer operands they emitted a
conditional branch after the left operand — skipping the right operand when
the result was already determined.

For floating-point operands the handlers did not call `emit_fp_bool_to_rax()`
before the branch; instead they fell through to the integer comparison path
(`cmp rax, 0` or `cmp eax, 0`), which compared the raw bits of `xmm0`'s
spilled value rather than the FP truth value.  As a result the branch was
taken or not taken based on garbage, and the RHS side effects always executed.

### Fix

In both the `&&` and `||` handlers, check the left operand's result type before
emitting the conditional branch, and call `emit_fp_bool_to_rax()` when it is FP:

```c
codegen_expression(cg, node->children[0]);
if (type_is_fp(node->children[0]->result_type)) {
    emit_fp_bool_to_rax(cg, node->children[0]->result_type);
    fprintf(cg->output, "    test rax, rax\n");
} else if (node->children[0]->result_type == TYPE_LONG) {
    fprintf(cg->output, "    cmp rax, 0\n");
} else {
    fprintf(cg->output, "    cmp eax, 0\n");
}
fprintf(cg->output, "    je .L_and_false_%d\n", label_id);
```

The same check is applied for the right operand for completeness (the right
operand result also needs FP-to-bool conversion before it is combined with the
short-circuit result).

`emit_fp_bool_to_rax()` is an existing helper that converts the `xmm0` value
to 0 or 1 in `rax` using `ucomisd`/`ucomiss` with a zero register.

---

## Files Changed

| File | Change |
|------|--------|
| `src/codegen.c` | `ArgSlot.is_fp`; `compute_call_layout` sets `is_fp`; Phase 1 FP stack arg path; `AST_INDIRECT_CALL` FP-aware path; `codegen_add_global` accepts `AST_ADDR_OF`; `codegen_emit_data` emits `dq label`; block-scope static addr-const bypass; `codegen_emit_local_statics` label-based emit; `&&`/`\|\|` FP short-circuit fix |
| `include/codegen.h` | `LocalStaticVar` gains `is_label_init` and `init_label` fields |
| `src/parser.c` | Global pointer initializer validation accepts `AST_ADDR_OF` |
| `src/version.c` | `VERSION_STAGE` bumped to `"01230000"` |

---

## Implementation Order

1. Add `is_fp` to `ArgSlot`; set it in `compute_call_layout()`.
2. Fix Phase 1 direct-call FP stack arg emission (CC99-001/002).
3. Add FP-aware path to `AST_INDIRECT_CALL` handler (CC99-003).
4. Extend `LocalStaticVar` with `is_label_init`/`init_label` (CC99-004).
5. Extend parser validation to accept `AST_ADDR_OF` (CC99-004).
6. Fix `codegen_add_global` and `codegen_emit_data` for `AST_ADDR_OF` (CC99-004).
7. Fix block-scope static addr-const bypass in `codegen_add_local_static` (CC99-004).
8. Fix `codegen_emit_local_statics` for `is_label_init` (CC99-004).
9. Fix `&&`/`||` FP short-circuit (CC99-005).
10. Build (`cmake --build build`).
11. Add 7 new tests (see below); run `./test/run_all_tests.sh`.
12. Bump `src/version.c`.
13. Self-host: `./build.sh --mode=self-host`.
14. Commit.

---

## Tests

All new tests go in `test/valid/`.  Expected exit code is in the filename suffix
(`__N`).

### CC99-001: Non-Variadic 9th Double Argument

**`test/valid/floating_point/test_fp_9th_arg_stack__45.c`**

```c
double sum9(
    double a1, double a2, double a3,
    double a4, double a5, double a6,
    double a7, double a8, double a9
) {
    return a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9;
}

int main(void) {
    return (int)sum9(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
}
```

Expected exit: **45**

### CC99-002: Variadic 9th Double Argument

**`test/valid/varargs/test_va_arg_double9__45.c`**

```c
#include <stdarg.h>

int sumd(int n, ...) {
    va_list ap;
    int i;
    double s = 0.0;
    va_start(ap, n);
    for (i = 0; i < n; i = i + 1)
        s = s + va_arg(ap, double);
    va_end(ap);
    return (int)s;
}

int main(void) {
    return sumd(9, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
}
```

Expected exit: **45**

### CC99-003: Indirect Call With Multiple Double Arguments

**`test/valid/functions/test_indirect_call_fp_args__9.c`**

```c
typedef double (*Fn2)(double, double);
typedef double (*Fn3)(double, double, double);

double add2(double a, double b) { return a + b; }
double add3(double a, double b, double c) { return a + b + c; }

int main(void) {
    Fn2 p2 = add2;
    Fn3 p3 = add3;
    return (int)(p2(1.0, 2.0) + p3(1.0, 2.0, 3.0));
}
```

Expected exit: **9**

### CC99-004A: Global Pointer Initialized From `&global`

**`test/valid/pointers/test_global_ptr_addr_const__37.c`**

```c
int x = 37;
int *p = &x;

int main(void) {
    return *p;
}
```

Expected exit: **37**

### CC99-004B: Global Pointer Initialized From `&global[N]`

**`test/valid/pointers/test_global_ptr_array_elem_addr__11.c`**

```c
int xs[3] = { 5, 7, 11 };
int *p = &xs[2];

int main(void) {
    return *p;
}
```

Expected exit: **11**

### CC99-004D: Block-Scope Static Pointer Initialized From `&global`

**`test/valid/pointers/test_static_ptr_addr_const__51.c`**

```c
int x = 51;

int main(void) {
    static int *p = &x;
    return *p;
}
```

Expected exit: **51**

### CC99-005: FP Logical Short-Circuit

**`test/valid/floating_point/test_fp_logical_short_circuit__5.c`**

```c
int main(void) {
    double a = 0.0;
    double b = 2.0;
    int x = 0;

    if (a && (x = 1)) {
        x = 2;
    }

    if (b || (x = 3)) {
        x = x + 5;
    }

    return x;
}
```

Expected exit: **5**

---

## Out of Scope

- **CC99-004C (global function pointer)** — `Fn fp = add;` already worked via
  the existing `AST_VAR_REF` path in the parser and the `dq function_name` emit
  path in `codegen_emit_data`.  Only the `AST_ADDR_OF` forms (subtypes A, B, D)
  required new code.
- **FP register save/restore across calls** — XMM registers are all caller-saved
  in SysV AMD64; no callee-save logic is needed.
- **Optimization of short-circuit with constant FP operands** — constant folding
  of FP boolean expressions is not in scope.
- **`float` stack-arg overflow** — the fix handles both `float` (4-byte
  `movss`) and `double` (8-byte `movsd`) stack-overflow FP args.  No separate
  stage is needed for `float`.

---

## Bootstrap Caveats

Stage 123 changes are purely behavioral for the programs being compiled; the
compiler's own assembly output structure is unchanged (no new instructions added
to the compiler's own prologue/epilogue).  Self-hosting proceeds normally:

- **C0 → C1**: GCC-compiled C0 compiles the stage-123 source into C1.
- **C1 → C2**: C1 compiles the same source into C2.  C1 and C2 must produce
  identical assembly for every test program.
- The test suite validates the generated programs' exit codes, not the generated
  assembly text.  No `test/print_asm/` updates are required for this stage.
