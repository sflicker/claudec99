# ClaudeC99 Stage 126–130 Roadmap

Derived from the post-stage-125 feature gap analysis.  
Work each stage in order: write spec → implement → update checklist.

---

## Stage 126 — Tentative Definitions

> **Note:** Stage 126 replaces the originally suggested "zero-initialization" — that
> feature is already implemented (uninitialized globals → BSS; partial initializers
> → zero-padded in `.data`). The real gap is tentative definitions.

- [x] Write spec `docs/stages/ClaudeC99-spec-stage-126-tentative-definitions.md`
- [x] Implement
- [x] Update `docs/outlines/checklist.md`

**What to build:** C99 §6.9.2 — a file-scope declaration without an initializer
is a *tentative definition*. Multiple tentative definitions of the same identifier
are legal; they merge into a single definition. A subsequent declaration with an
initializer completes it; a tentative definition with no completing definition is
treated as if initialized to zero.

**Currently failing:**
```c
int x;        /* tentative definition */
int x = 5;   /* completes it — currently: "duplicate global declaration 'x'" */
```

---

## Stage 127 — Callee-Saved Registers r12–r15

- [x] Write spec `docs/stages/ClaudeC99-spec-stage-127-callee-saved-r12-r15.md`
- [x] Implement
- [x] Update `docs/outlines/checklist.md`

**What to build:** SysV AMD64 ABI requires r12, r13, r14, r15 to be preserved
across calls. Currently only rbx is saved (Stage 122). Any function that the
compiler allocates r12–r15 into (or that the callee uses internally) silently
corrupts those registers. Fix: push/pop r12–r15 in every function prologue/epilogue,
matching the existing rbx pattern.

---

## Stage 128 — Floating-Point Constant Expressions at File Scope

- [x] Write spec `docs/stages/ClaudeC99-spec-stage-128-fp-constant-expressions.md`
- [x] Implement
- [x] Update `docs/outlines/checklist.md`

**What to build:** File-scope float/double initializers that are constant
expressions involving arithmetic on FP literals. Currently only a single FP
literal is accepted.

**Currently failing:**
```c
double TWO_PI = 3.14159 * 2.0;   /* error: requires a constant initializer */
double NEG_ONE = -1.0;            /* works (unary minus + float literal) */
double SUM = 1.0 + 0.5;          /* fails */
```

---

## Stage 129 — Function Declarations at Block Scope and Extern Incomplete Arrays

- [x] Write spec `docs/stages/ClaudeC99-spec-stage-129-block-scope-fn-decls-and-incomplete-extern-arrays.md`
- [x] Implement
- [x] Update `docs/outlines/checklist.md`

**What to build:** Two small C99 features:

1. **Function declarations at block scope** — `void foo(void);` inside `{}` is
   legal C99. Currently the parser rejects it with "expected ';', got '('".

2. **Incomplete array types in `extern` declarations** — `extern int arr[];`
   is legal when `arr` is defined elsewhere with a concrete size. Currently
   the parser errors: "array size required for file-scope declaration 'arr'".

---

## Stage 130 — `va_arg` for Struct-by-Value Types

- [x] Write spec `docs/stages/ClaudeC99-spec-stage-130-vaarg-struct-by-value.md`
- [x] Implement
- [x] Update `docs/outlines/checklist.md`

**What to build:** `va_arg(ap, struct T)` — extract a struct passed by value
from a variadic argument list. Requires applying the SysV AMD64 register-class
classification at the `va_arg` call site (same logic already used for struct
arguments in `compute_call_layout`). A ≤16-byte register-class struct is read
from the GP/XMM registers in the `va_list`; a memory-class struct is read from
the overflow area via the `overflow_arg_area` pointer.

**Currently failing:**
```c
struct Point p = va_arg(ap, struct Point);
/* error: expression is not a usable struct/union value */
```
