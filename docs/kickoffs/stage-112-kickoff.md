# Stage 112 Kickoff — `float`/`double` Calling Convention, `va_arg`, and `<math.h>`

**Objective**: Complete floating-point support by implementing the SysV AMD64 calling convention for FP arguments and return values, extending `va_arg` for `double`, and providing a `<math.h>` stub.

**Spec**: `docs/stages/ClaudeC99-spec-stage-112-float-double-calls-math.md`

---

## Summary

Stage 112 implements the complete SysV AMD64 calling convention for floating-point types:

1. **Non-variadic calls with FP arguments** (Task 1): FP args placed in `xmm0`–`xmm7` (SSE class); integer and SSE slots counted independently.
2. **FP return values** (Task 3): float/double return values arrive in `xmm0`; no extra move needed.
3. **Non-variadic function prologues** (Task 2): FP parameters moved from `xmmN` to local stack slots via `movsd`/`movss`.
4. **Variadic calls** (Task 4): `al` register set to `<xmm_arg_count>` before calling variadic functions.
5. **Variadic prologues** (Task 5): Save `xmm0`–`xmm7` (16 bytes each via `movaps`) to register-save area; grow from 48 to 176 bytes.
6. **`va_arg` for `double`** (Task 6): Read from register-save area (`fp_offset` slot) or overflow stack.
7. **`<math.h>` stub** (Task 7): Function declarations for trig, exp, log, power, and rounding functions; constants `M_PI`, `M_E`, etc.
8. **Version bump** (Task 8): `VERSION_STAGE = "01120000"`.

---

## Tokenizer Changes

None required.

---

## Parser Changes

None required.

---

## AST Changes

None required.

---

## Code Generation Changes

**`src/codegen.c`** — all FP calling convention work:

1. **Extend argument classification** (Task 1): Add `xmm_arg_count` parallel to `int_arg_count`; stage FP arguments into `xmm0`–`xmm7` via `movsd`/`movss`.

2. **Parameter prologues** (Task 2): For each FP parameter, emit `movsd [rbp - N], xmmK` or `movss [rbp - N], xmmK` during function entry.

3. **Return values** (Task 3): FP expressions evaluated into `xmm0`; return instructions (`pop rbp; ret`) follow automatically.

4. **Variadic call-site `al`** (Task 4): Emit `mov al, <xmm_arg_count>` immediately before `call` to variadic functions.

5. **Variadic prologue XMM saves** (Task 5):
   - Change register-save area from 48 to 176 bytes.
   - After GP register saves, emit `movaps [rbp - <rsave> + 48+16*K], xmmK` for K = 0..7.
   - Ensure 16-byte alignment: `[rbp - <rsave> + 48]` must be 16-byte aligned.

6. **`va_arg` for `double`** (Task 6): Add `TYPE_DOUBLE` case to `va_arg` codegen; load from `reg_save_area + fp_offset` or overflow area; advance `fp_offset` by 16.

**Header changes**:
- `include/codegen.h`: Add `variadic_named_xmm_params` field to `CodeGen` struct (tracks XMM params in non-variadic context; used to set initial `fp_offset` in `va_start`).

---

## Test Plan

7 test files in `test/valid/`:

1. **`test_fp_func_call__0.c`** — function taking and returning `double`
2. **`test_fp_mixed_params__0.c`** — function with mixed `int`/`double` parameters
3. **`test_fp_return_float__0.c`** — function returning `float`
4. **`test_fp_sqrt__0.c`** — call `sqrt` from `<math.h>`
5. **`test_fp_printf__0.c`** — `printf` with `double` argument (variadic FP)
6. **`test_fp_varargs__0.c`** — `va_arg` for `double`
7. **`test_fp_pow__0.c`** — call `pow` from `<math.h>`

Expected exit code: 0 for all (optional stdout capture for `test_fp_printf__0.c`).

---

## Implementation Order

1. Non-variadic FP parameter prologue (Task 2)
2. Non-variadic FP call-site argument staging (Task 1)
3. FP return values (Task 3)
4. Variadic call `al` register (Task 4)
5. Variadic prologue XMM register save (Task 5)
6. `va_arg` for `double` (Task 6)
7. Create `test/include/math.h` stub (Task 7)
8. Version bump (Task 8)
9. Write test files
10. Build, run full test suite, verify self-host cycle

---

## Key Design Notes

- **Calling convention alignment**: SSE-class arguments are passed in `xmm0`–`xmm7` independently of integer slots. Register-save area base (offset 48 in `va_list`) must be 16-byte aligned.
- **Argument staging**: Evaluate each FP argument into `xmm0`, then move to target `xmmK` (e.g., `movsd xmm1, xmm0`).
- **Variadic `al` convention**: Required by ABI; must be set on every call to a variadic function.
- **XMM save optimization**: The spec notes that ABI allows saving only the first `al` XMM registers; ClaudeC99 saves all 8 unconditionally for simplicity.
- **`float` in variadic context**: C99 §6.5.2.2p6 requires float promotion to double; `va_arg(ap, float)` is undefined. Do not implement; emit compile error if used.

---

## Out of Scope

- `va_arg(ap, float)` — emit compile error.
- `long double` — deferred indefinitely.
- `<complex.h>`, `<fenv.h>` — deferred.
- `isnan`/`isinf`/`isfinite` as macro expressions — declared as functions in stub.
- `HUGE_VAL`, `INFINITY`, `NAN` constants — stub declarations only; remove if builtins fail.
- Struct-returning math functions — already in `<stdlib.h>`.

---

## Next Steps

1. Read current `src/codegen.c` structure (ArgSlot, compute_call_layout, prologue paths, call-site paths, va_arg).
2. Extend ArgSlot and compute_call_layout for FP argument tracking.
3. Implement tasks in order.
4. Pause after each task for validation.
