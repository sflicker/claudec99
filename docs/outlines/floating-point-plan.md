# ClaudeC99 Floating-Point Implementation Plan — Stages 109–112

_Drafted 2026-06-13_

## Overview

Floating-point support (`float` and `double`) is the largest remaining
language gap in ClaudeC99. It touches every compiler layer — the lexer,
the type system, the parser, and the code generator — and requires
following the SysV AMD64 ABI's SSE2 calling convention. The work is
split into four independent, shippable stages so that each stage can be
self-hosted and tested before the next begins.

`long double` (x87 80-bit) is explicitly deferred; its stack semantics
are significantly different from SSE2 and the practical benefit is low.

---

## Stage 109 — Types, Literals, and Stack Variables

**Spec**: `docs/stages/ClaudeC99-spec-stage-109-float-double-types-literals.md`

**Scope**: everything needed to declare and load/store float/double values;
no arithmetic, no comparisons, no function calls.

- **Lexer**: `TOKEN_FLOAT`, `TOKEN_DOUBLE`; float/double literal lexing
  (decimal `1.5`, `.5`, `1.0e-3`; suffixes `f`/`F` for float, no suffix
  for double).
- **AST**: new node `AST_FLOAT_LITERAL` carrying the raw literal text.
- **Type system**: `TYPE_FLOAT`, `TYPE_DOUBLE` added to `include/type.h` /
  `src/type.c`; `sizeof(float) == 4`, `sizeof(double) == 8`.
- **Parser**: `float`/`double` in `parse_type_specifier`; float literals
  in `parse_primary`.
- **Codegen**:
  - Stack frame: 4-byte slots for `float` (4-byte aligned), 8-byte slots
    for `double` (8-byte aligned).
  - FP expression results land in `xmm0` (analogous to `rax` for integers).
  - Load: `movss xmm0, [rbp - N]` / `movsd xmm0, [rbp - N]`.
  - Store: `movss [rbp - N], xmm0` / `movsd [rbp - N], xmm0`.
  - Float literals: emitted to `.rodata` as `DD` (float) / `DQ` (double)
    with a generated label; referenced via `movss xmm0, [rel Lfc_N]`.
- **No** arithmetic, comparisons, or function calls with FP operands yet.

**Deliverable**: `float x = 1.5f; double y = x;` compiles and runs.

---

## Stage 110 — FP Arithmetic, Conversions, and Casts

**Spec**: `docs/stages/ClaudeC99-spec-stage-110-float-double-arithmetic.md`

**Scope**: expressions involving float/double operands; implicit and
explicit conversions between integer and FP types.

- **Codegen — binary arithmetic** (left in `xmm0`, right in `xmm1`,
  result in `xmm0`):
  - `addss`/`addsd`, `subss`/`subsd`, `mulss`/`mulsd`, `divss`/`divsd`.
  - Binary op evaluation: push `xmm0` to stack (`sub rsp, 8` +
    `movsd [rsp], xmm0`) before evaluating the right operand into `xmm0`;
    pop into `xmm1` (`movsd xmm1, [rsp]` + `add rsp, 8`); then emit op.
- **Unary minus**: `xorpd xmm0, [rel Lsign_mask_f64]` (sign-bit mask in
  `.rodata`); `float` uses `xorps`.
- **Usual Arithmetic Conversions for FP** (C99 §6.3.1.8):
  `int op float → float`, `float op double → double`,
  `int op double → double`.
- **Implicit conversions**:
  - int → float: `cvtsi2ss xmm0, rax`
  - int → double: `cvtsi2sd xmm0, rax`
  - float → double: `cvtss2sd xmm0, xmm0`
  - double → float: `cvtsd2ss xmm0, xmm0`
- **Explicit casts** (`(float)`, `(double)`, `(int)` on FP values):
  - FP → int: `cvttss2si rax, xmm0` / `cvttsd2si rax, xmm0` (truncating).
- **`sizeof(float)` = 4, `sizeof(double)` = 8** (already from Stage 109).

**Deliverable**: `double d = (double)n / 3.0; float f = (float)d + 1.5f;`
compiles and produces correct results.

---

## Stage 111 — FP Comparisons and Boolean Contexts

**Spec**: `docs/stages/ClaudeC99-spec-stage-111-float-double-comparisons.md`

**Scope**: using float/double in comparison expressions and control-flow
conditions.

- **Instruction**: `ucomiss xmm0, xmm1` (float) / `ucomisd xmm0, xmm1`
  (double). Sets CF, ZF, PF (PF = 1 for unordered/NaN).
- **Comparison operators** — load left into `xmm0`, right into `xmm1`,
  then emit `ucomiss`/`ucomisd` and set `al` via:

  | Op  | Condition code(s) | Notes |
  |-----|-------------------|-------|
  | `<` | `setb al`         | CF=1 |
  | `<=`| `setbe al`        | CF=1 or ZF=1 |
  | `>` | `seta al`         | CF=0 and ZF=0 |
  | `>=`| `setae al`        | CF=0 |
  | `==`| `sete al; setnp cl; and al, cl` | equal AND not unordered |
  | `!=`| `setne al; setp cl; or al, cl`  | not-equal OR unordered |

- **Boolean context** (`if`, `while`, `for`, `?:` condition on FP value):
  `xorpd xmm1, xmm1` (zero); `ucomisd xmm0, xmm1`; `setne al; setp cl;
  or al, cl`; `movzx rax, al`.
- **Logical `!` on FP**: same as boolean context, then `xor al, 1`.

**Deliverable**: FP comparisons and control-flow work; a bubble-sort over
a `double[]` compiles and produces correct results.

---

## Stage 112 — FP Calling Convention, `va_arg`, and `<math.h>`

**Spec**: `docs/stages/ClaudeC99-spec-stage-112-float-double-calls-math.md`

**Scope**: calling functions with FP arguments; returning FP values;
variadic calls with FP arguments; `va_arg` for `double`; `<math.h>` stub.

### Non-variadic calls
- SysV AMD64: FP args go in `xmm0`–`xmm7` (tracked independently from
  integer arg slots in `rdi`–`r9`).
- Call-site: extend the argument-classification loop with a parallel
  `xmm_arg_count` counter; emit FP args as `movsd [rsp + N], xmm0` or
  directly into `xmm0`–`xmm7`.
- Prologue: FP parameters read from `xmm0`–`xmm7` into local stack slots
  at function entry.
- FP return value arrives in `xmm0`; caller moves it to the local stack
  slot as needed.

### Variadic calls
- `al` must contain the count of `xmm` registers used before each `call`
  to a variadic function.
- Extend call-site codegen to track `xmm_arg_count` and emit `mov al,
  <count>` before the `call`.

### `va_arg` for `double`
- C99 §6.5.2.2p6: `float` arguments are promoted to `double` in variadic
  calls — callers always pass `double`.
- Extend the variadic function prologue to save `xmm0`–`xmm7` into the
  register-save area (16 bytes each, starting at `reg_save_area + 48`).
- `va_arg(ap, double)`: read `ap->fp_offset`; if `< 176` (48 + 8×16),
  load from `reg_save_area + fp_offset`, advance `fp_offset += 16`;
  otherwise load from `overflow_arg_area` and advance it.
- `va_arg(ap, float)` is undefined by C99 (float is always promoted);
  ClaudeC99 may error or treat it as `double`.

### `test/include/math.h` stub
Single-precision and double-precision declarations for:
`sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `sinh`, `cosh`,
`tanh`, `exp`, `log`, `log2`, `log10`, `pow`, `sqrt`, `cbrt`, `fabs`,
`fabsf`, `floor`, `floorf`, `ceil`, `ceilf`, `round`, `roundf`, `fmod`,
`fmodf`, `fmin`, `fmax`, `fminf`, `fmaxf`, `isnan`, `isinf`, `isfinite`.
The `float` variants use the `f` suffix.

**Deliverable**: `printf("%.2f\n", sqrt(2.0))` compiles and prints `1.41`.

---

## Key design decisions across all four stages

### FP result register convention
Integer/pointer expression results land in `rax`; FP results land in
`xmm0`. The codegen tracks the result type of each expression and emits
the appropriate move/store. Binary operator codegen saves `xmm0` to the
stack (via `sub rsp, 8` + `movsd [rsp], xmm0`) when it needs to preserve
the left-hand side while evaluating the right-hand side — analogous to
the existing `push rax` / `pop rcx` pattern for integer binary ops.

### Float literals in `.rodata`
NASM accepts bare decimal values after `DD` and `DQ` directives
(`DD 1.5` emits a 4-byte IEEE 754 float; `DQ 1.5` emits an 8-byte
double). Literals are deduplicated by value using a flat table per
translation unit; each emits a label `Lfc_N` in `.rodata` and is
referenced via `[rel Lfc_N]`.

### `long double` deferred
x87 80-bit extended precision uses a completely different register file
(`st0`–`st7`) and stack-based operations. It is out of scope for all four
stages.

### Interaction with self-hosting
Stages 109–112 should each pass the full self-host C0→C1→C2 cycle before
the next stage begins. The compiler source (`src/*.c`) uses no
floating-point itself, so the self-host cycle remains clean throughout.
