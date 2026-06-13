# Stage 111 Kickoff: Float/Double Comparisons and Boolean Contexts

## Spec Summary

Stage 111 enables float and double values in comparison expressions, logical operators, and control-flow conditions. After Stage 110 (which added FP arithmetic), FP values can now be compared with `<`, `<=`, `>`, `>=`, `==`, `!=`, used as conditions in `if`, `while`, and ternary operators (`?:`), and negated with logical NOT (`!`). When comparing float/double against integers, the integer is converted to FP before comparison per C99 Usual Arithmetic Conversions (UAC).

**Scope**:
- Codegen: SSE2 comparison instructions (`ucomiss`/`ucomisd`) with proper NaN handling
- FP boolean contexts: convert FP to 0/1 for use in conditions, with NaN treated as true
- Logical NOT on FP: `!fp_value` returns 1 if value is zero (and not NaN), 0 otherwise
- Mixed FP/int comparisons: convert integer to FP before comparison
- Version bump to `01110000`
- Tests: 8 new tests covering comparisons, conditions, logical NOT, ternary, and mixed-type comparisons

---

## Files to Change

| File | Changes |
|------|---------|
| `src/codegen.c` | FP comparison operators (`<`, `<=`, `>`, `>=`, `==`, `!=`); FP in boolean contexts; logical NOT on FP; mixed FP/int comparison conversions |
| `src/version.c` | Bump stage to `01110000` |
| (Tokenizer: no changes) | All tokens already exist |
| (Parser: no changes) | All productions already accept FP expressions |
| (AST: no changes) | All nodes already differentiate FP types via `TYPE_FLOAT` and `TYPE_DOUBLE` |

---

## Key Design Decisions

### 1. FP Comparison Instructions and Flag Interpretation

SSE2 provides unordered compare instructions that set CPU flags:

```nasm
ucomiss xmm0, xmm1    ; compare float  (left=xmm0, right=xmm1)
ucomisd xmm0, xmm1    ; compare double (left=xmm0, right=xmm1)
```

**Flag behavior**:
- **ZF=1, PF=0, CF=0**: `xmm0 == xmm1` (ordered equal)
- **ZF=0, PF=0, CF=1**: `xmm0 < xmm1` (ordered less-than)
- **ZF=0, PF=0, CF=0**: `xmm0 > xmm1` (ordered greater-than)
- **ZF=1, PF=1, CF=1**: unordered (at least one operand is NaN)

**Instruction selection per comparison operator**:

| C operator | `set*` sequence | Notes |
|-----------|-----------------|-------|
| `<`  | `setb al` | CF=1 |
| `<=` | `setbe al` | CF=1 or ZF=1 |
| `>`  | `seta al` | CF=0 and ZF=0 |
| `>=` | `setae al` | CF=0 |
| `==` | `sete al` ; `setnp cl` ; `and al, cl` | equal AND not NaN |
| `!=` | `setne al` ; `setp cl` ; `or al, cl`  | not-equal OR NaN |

For `==`: C99 §6.5.9p4 requires that any comparison involving NaN returns false/zero (except `!=`). `sete` alone would return 1 for `NaN == NaN` (ZF=1 in the unordered case), so AND with `setnp` (not-parity = not NaN) is required to suppress the false positive.

For `!=`: C99 §6.5.9p4 specifies that `NaN != NaN` is true. `setne` alone would return 0 for `NaN != NaN` (ZF=1 in the unordered case), so OR with `setp` (parity = NaN) is required to ensure the correct result.

After the `set*` sequence, `movzx rax, al` zero-extends the 1/0 byte into the full result register (TYPE_INT).

### 2. Operand Loading Convention

Evaluate the left operand into `xmm0`; save to stack; evaluate the right operand into `xmm0`; restore left into `xmm1`; then emit `ucomiss xmm1, xmm0`. This places left in `xmm1` and right in `xmm0`, which is the standard argument order: the instruction `ucomiss/ucomisd xmm1, xmm0` compares left against right.

### 3. FP in Boolean Contexts

Any scalar value used as a condition in C is "true" if non-zero. For FP, a zero value (including `-0.0`) is false; any non-zero value is true. **NaN policy**: C99 leaves this unspecified, but ClaudeC99 treats NaN as true in boolean context (using `setne + setp` pattern), which is a valid implementation choice.

**Codegen** for an FP value in a condition slot (e.g., `if (x)` where `x` is `float` or `double`):

```nasm
; value in xmm0
xorpd xmm1, xmm1              ; zero in xmm1 (for double)
ucomisd xmm0, xmm1            ; compare value against 0.0
setne al                      ; non-zero?
setp  cl                      ; NaN? (NaN is also "true" here)
or    al, cl
movzx rax, al                 ; rax = 1 if (value != 0.0 || NaN), else 0
```

For `float`, use `xorps`/`ucomiss` instead. This value in `rax` is then tested as an integer condition by the existing condition-code testing in `if`/`while`/etc.

**Where to emit**: 
- Modify `emit_cond_cmp_zero()` helper to detect FP condition nodes and emit the above sequence before the `cmp eax, 0`
- Update `AST_CONDITIONAL_EXPR` (`?:`) handling to call `emit_cond_cmp_zero()` for FP conditions

### 4. Logical NOT on FP

`!fp_value` returns 1 if the value is zero (and not NaN), 0 otherwise. Per C99, `!NaN` = 0 (NOT of an unordered value is false).

```nasm
; value in xmm0 (double)
xorpd xmm1, xmm1
ucomisd xmm0, xmm1
sete  al           ; equal to zero?
setnp cl           ; not NaN?
and   al, cl       ; zero AND not NaN
movzx rax, al
```

For `float`, use `xorps`/`ucomiss`. Emit this logic in the unary `!` operator handling in `codegen_expression`, before the integer NOT path.

### 5. Mixed FP/int Comparisons

When comparing a float/double against an integer expression, the integer must be converted to FP before the comparison (C99 §6.3.1.8). Use the type resolution helper `fp_common_arith_kind()` (from Stage 110) to determine the common FP type, then emit:

```nasm
cvtsi2sd xmm1, rax    ; int → double before ucomisd
```

for integer-to-double conversion, or `cvtsi2ss` for integer-to-float. This logic integrates naturally into the FP comparison block in `AST_BINARY_OP` by widening operands before the compare instruction.

### 6. Version Bump

Update `src/version.c`:
```c
#define VERSION_STAGE "01110000"
```

---

## Implementation Order

1. **Codegen: FP comparison block in `AST_BINARY_OP`** — handle `==`, `!=`, `<`, `<=`, `>`, `>=` when either operand is FP; use save/restore convention; emit `set*` sequences per operator; convert integer operands to FP via `cvtsi2ss`/`cvtsi2sd`
2. **Codegen: FP boolean context via helper** — create `emit_fp_bool_to_rax()` helper; modify `emit_cond_cmp_zero()` to detect and call it for FP conditions; update `AST_CONDITIONAL_EXPR` handling
3. **Codegen: logical NOT on FP** — extend unary `!` operator path to handle FP before integer path
4. **Version bump** to `01110000`
5. **Tests** — 8 new test cases
6. **Build, full test suite, self-host cycle**

---

## Spec Ambiguities and Clarifications

1. **NaN handling in bare boolean context**: C99 §6.3.1.2 leaves the behavior unspecified when a NaN is converted to `_Bool`. ClaudeC99 treats NaN as true (via `setne + setp`), which is one valid interpretation. This differs from C99's explicit false behavior for `==`, `<`, etc., but is consistent with treating any non-zero bit pattern as true.

2. **Operator precedence and associativity**: No changes; FP comparisons use the same precedence and associativity as integer comparisons.

3. **Comparison result type**: All comparisons return `TYPE_INT` (0 or 1), consistent with C99 §6.5.8 and §6.5.9.

4. **FP function parameters and return values**: Deferred to Stage 112. Stage 111 does not handle passing FP values to/from functions.

5. **`isnan()` / `isinf()` library functions**: Deferred to Stage 112.

6. **Floating-point exceptions (`fenv.h`)**: Deferred.

7. **`long double`**: Deferred.

---

## Summary of Deliverables

- Kickoff (this document)
- Milestone summary (after testing passes)
- Transcript (after all implementation and testing)
- Grammar update (if FP operators need annotation; likely minimal)
- Checklist entry (Stage 111 section)
- README update (through-stage line, FP comparison support, test totals)
- Self-compilation report (Stage 111 self-host run after tests pass)
- Floating-point plan update (plan vs. actual notes)
