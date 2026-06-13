# Stage 110 Kickoff: Float/Double Arithmetic, Conversions, and Casts

## Spec Summary

Stage 110 enables full arithmetic on `float` and `double` values and implements all C99 type-conversion rules governing mixed-type expressions. After Stage 109 (which added FP types, literals, and load/store), binary operators on FP operands now emit SSE2 instructions (`addss`/`addsd`/`subss`/`subsd`/`mulss`/`mulsd`/`divss`/`divsd`). Usual Arithmetic Conversions (UAC) ensure that mixed-type operations (`float + int`, `float + double`, etc.) produce correct types. Unary minus, explicit casts (FP ↔ FP, FP ↔ int), and implicit int-to-FP conversions in assignments complete the arithmetic subsystem.

**Scope**:
- Codegen: SSE2 binary arithmetic with stack-based FP operand save/restore
- UAC type resolution in binary-op codegen helpers (`expr_result_type`, `sizeof_type_of_expr`)
- Unary minus for FP via XOR with sign-bit mask constants in `.rodata`
- Explicit casts: `cvtsi2ss`/`cvtsi2sd` (int→FP), `cvtss2sd`/`cvtsd2ss` (FP↔FP), `cvttss2si`/`cvttsd2si` (FP→int, truncate-toward-zero)
- Implicit int→FP in assignments and initializers
- Version bump to `01100000`
- Tests: 8 new tests covering arithmetic ops, UAC, unary minus, and casts

---

## Files to Change

| File | Changes |
|------|---------|
| `src/codegen.c` | FP binary arithmetic ops, UAC type resolution, sign-mask constants, unary minus, explicit/implicit casts |
| `include/codegen.h` | Flags for sign-mask emission: `fp_sign_mask_f32_emitted`, `fp_sign_mask_f64_emitted` |
| `src/version.c` | Bump stage to `01100000` |
| (Parser: no changes) | UAC resolution remains in codegen, not parser |

---

## Key Design Decisions

### 1. FP Binary Arithmetic — SSE2 Instructions and Register Convention

For binary operations on FP operands:
- Left operand is evaluated first, result in `xmm0`
- Before evaluating the right operand, save `xmm0` to stack: `sub rsp, 8; movsd [rsp], xmm0` (or `movss` for float)
- Evaluate right operand; result lands in `xmm0`
- Restore left operand from stack into `xmm1`: `movsd xmm1, [rsp]; add rsp, 8`
- After restore: `xmm1` = left, `xmm0` = right
- Emit instruction with non-commutative operations using left operand in `xmm1`: `subsd xmm1, xmm0`
- For commutative ops (`addsd`, `mulsd`), order is irrelevant
- Result in `xmm0` via `movsd xmm0, xmm1` (or equivalently, define the instruction to operate in reverse for commutative ops)

**Instruction selection table**:

| Op | float | double |
|----|-------|--------|
| `+` | `addss xmm0, xmm1` | `addsd xmm0, xmm1` |
| `-` | `subss xmm1, xmm0; movss xmm0, xmm1` | `subsd xmm1, xmm0; movsd xmm0, xmm1` |
| `*` | `mulss xmm0, xmm1` | `mulsd xmm0, xmm1` |
| `/` | `divss xmm1, xmm0; movss xmm0, xmm1` | `divsd xmm1, xmm0; movsd xmm0, xmm1` |

### 2. Usual Arithmetic Conversions (UAC)

In `codegen.c` functions `expr_result_type()` and `sizeof_type_of_expr()`:
- If either operand is `double`, result is `double`
- Else if either operand is `float`, result is `float`
- These checks occur **before** integer promotion rules
- Before emitting the binary instruction, widen operands as needed:
  - `float` → `double`: `cvtss2sd xmm0, xmm0` (or the operand's register)
  - `int` (in `rax`) → `float`: `cvtsi2ss xmm0, rax`
  - `int` (in `rax`) → `double`: `cvtsi2sd xmm0, rax`

### 3. Unary Minus on FP

SSE2 has no negate instruction. Use XOR with a sign-bit mask:
- Float sign-mask constant: `0x80000000` (1 bit in bit 31)
- Double sign-mask constant: `0x8000000000000000` (1 bit in bit 63)
- Emit to `.rodata` as `Lfp_smask_f32` and `Lfp_smask_f64` (local labels)
- Emit on first use (track via flags `fp_sign_mask_f32_emitted` and `fp_sign_mask_f64_emitted` in `CodeGen` struct)
- Instruction: `xorps xmm0, [rel Lfp_smask_f32]` (float) or `xorpd xmm0, [rel Lfp_smask_f64]` (double)

### 4. Explicit Casts

Extend `AST_CAST` codegen to handle:

| From | To | Instruction |
|------|-----|-------------|
| int/long (rax) | float | `cvtsi2ss xmm0, rax` |
| int/long (rax) | double | `cvtsi2sd xmm0, rax` |
| float (xmm0) | double | `cvtss2sd xmm0, xmm0` |
| double (xmm0) | float | `cvtsd2ss xmm0, xmm0` |
| float (xmm0) | int/long | `cvttss2si rax, xmm0` |
| double (xmm0) | int/long | `cvttsd2si rax, xmm0` |

The `cvtt*` variants truncate toward zero (C99 §6.3.1.4 requirement).

### 5. Implicit int→FP in Assignments and Initializers

When the RHS is an integer and LHS is float/double, emit the conversion before the store:
- `AST_ASSIGNMENT` with mismatched types
- `AST_DECLARATION` with initializer where RHS is int and LHS is FP
- These use the same `cvtsi2ss`/`cvtsi2sd` instructions as explicit casts

### 6. Version Bump

Update `src/version.c`:
```c
#define VERSION_STAGE "01100000"
```

---

## Implementation Order

1. **Parser-side UAC validation** (if needed, likely no changes required; verify that `TOKEN_FLOAT`/`TOKEN_DOUBLE` are in lookahead sets)
2. **Codegen: sign-mask constants** — emit `.rodata` labels `Lfp_smask_f32` and `Lfp_smask_f64` on demand
3. **Codegen: FP binary arithmetic** — implement instruction selection and operand save/restore
4. **Codegen: unary minus for FP** — XOR with sign-mask
5. **Codegen: explicit casts** — handle all FP ↔ integer and FP ↔ FP conversions
6. **Codegen: implicit int→FP** — emit conversions in assignment/initializer paths
7. **Version bump**
8. **Tests** — 8 new test cases
9. **Build, full test suite, self-host cycle**

---

## Spec Ambiguities and Clarifications

1. **Parser-side UAC**: Spec mentions "Parser-side UAC type resolution," but in practice, binary-operator type inference is performed entirely in `codegen.c` via `expr_result_type()` and `sizeof_type_of_expr()` helpers. No parser.c changes are necessary; the type system already distinguishes `TYPE_FLOAT` and `TYPE_DOUBLE`.

2. **Sign-mask label naming**: Spec uses `Lfp_sign_mask_f32/f64`; implementation uses shorter `Lfp_smask_f32/Lfp_smask_f64` as local `.rodata` labels.

3. **Function return int→FP**: C99 allows implicit int-to-FP conversion in function return statements (if the function returns float/double). This is deferred to Stage 112 when FP function parameters and return values are implemented. The conversion instruction (`cvtsi2ss`/`cvtsi2sd`) is identical to explicit casts.

4. **FP function parameters and return**: Deferred to Stage 112. Stage 110 does not handle passing FP values to/from functions.

5. **Remainder operator (`%`)**: C99 leaves `%` undefined for FP types; skip entirely.

6. **Long double**: Deferred indefinitely.

---

## Summary of Deliverables

- Kickoff (this document)
- Milestone summary (after testing passes)
- Transcript (after all implementation and testing)
- Grammar update (FP operators in expression productions, if needed)
- Checklist entry (Stage 110 section)
- README update (through-stage line, FP arithmetic support, test totals)
