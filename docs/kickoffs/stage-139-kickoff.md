# Stage 139 Kickoff — Preprocessor `#if` Expression Gaps

## Summary

Fix three deficiencies in the `#if`/`#elif` constant-expression evaluator that prevent parsing system headers. All changes are confined to `src/preprocessor.c`, with version bump and new test cases.

**Goal:** Enable the compiler to parse glibc system headers by fixing:
1. **PP-01**: Integer literal suffixes (`L`, `U`, `LL`, `UL`, `ULL`, etc.) and hex literals (`0x...`) 
2. **PP-02**: Function-like macros inside `#if` expressions  
3. **PP-03**: Ternary operator `?:` in `#if` expressions

---

## Tokenizer Changes

**None.** No changes to tokenization.

---

## Parser Changes

**None.** No changes to parsing.

---

## AST Changes

**None.** No changes to the AST.

---

## Code Generation Changes

**None.** No changes to codegen.

---

## Test Plan

### New Integration Tests

Add 8 new test directories under `test/integration/`:

#### PP-01: Integer literal suffixes

1. **`test_pp_if_L_suffix`** — `201710L > 199901L` (both L-suffixed)
2. **`test_pp_if_UL_suffix_in_parens`** — `(1UL << 31) > 0` (suffix in parentheses)
3. **`test_pp_if_hex_constant`** — `0x600 >= 0x200` (hex literals)

#### PP-02: Function-like macros

4. **`test_pp_if_funclike_macro`** — `#define ATLEAST(maj, min) ...` with `#if ATLEAST(4, 6)`
5. **`test_pp_if_funclike_token_paste`** — token pasting: `#define CAT(a, b) a ## b` with nested expansion
6. **`test_pp_if_funclike_zero`** — function-like macro returning 0

#### PP-03: Ternary operator

7. **`test_pp_if_ternary_true`** — `#if FLAG ? 1 : 0` (FLAG = 1)
8. **`test_pp_if_ternary_defined`** — `#if defined __cplusplus ? ... : defined __USE_ISOC11` (glibc pattern)

All tests return exit code 0 on success.

### System Header Validation

Add `test/integration/run_tests_sysinclude.sh` — a copy of `run_tests.sh` with `DEFAULT_IFLAGS` changed to use real system include paths (`/usr/include`, etc.). Manual validation tool; not part of standard CI.

---

## Implementation Order

1. **`src/preprocessor.c` — PP-01**: Fix `eval_cond_primary` to consume integer suffixes and support hex literals
   - Add hex literal branch: `0x...` or `0X...`
   - Add suffix-consuming loop at end

2. **`src/preprocessor.c` — PP-03**: Add `eval_cond_ternary` function
   - Insert between current `eval_cond_logical_or` and `eval_cond_expr`
   - Update `eval_cond_expr` to call `eval_cond_ternary` instead of `eval_cond_logical_or`
   - Update forward declarations

3. **`src/preprocessor.c` — PP-02**: Pre-expand `#if`/`#elif` condition text
   - Extract raw condition text before `\n`
   - Pass through `expand_macros_text()`
   - Evaluate expanded result
   - Remove function-like macro guard (`if (m->param_count != -1) exit(1)`)
   - Repeat for all `#elif` handlers

4. **`src/version.c`**: Bump `VERSION_STAGE` from `"13800000"` to `"13900000"`

5. **Add 8 new test directories** with `.c` files and `.exit` files

6. **Add `test/integration/run_tests_sysinclude.sh`** script

7. **Full test run**: `./build.sh` and `./test/integration/run_tests_sysinclude.sh`

8. **Self-host**: `./build.sh --mode=self-host`

---

## Key Specification Notes

- **PP-02 correctness**: C99 §6.10.1 mandates macro expansion before `#if` evaluation. The `defined` operator is never in the macro table, so it passes through `expand_macros_text()` unchanged. Identifiers inside `defined(X)` are looked up directly in the evaluator (correct behaviour).

- **Ternary precedence**: Lower than `||`; associates right-to-left. Implementation: `eval_cond_ternary` calls itself recursively for the else-branch.

- **Test `test_pp_if_ternary_defined` correction**: Don't define `__cplusplus`; define only `__USE_ISOC11 1`. Then `defined __cplusplus` is false, ternary picks the else-branch `defined __USE_ISOC11` which is 1 → `#if` true → exit 0.

- **System header failures after stage**: 7 remaining failures are unrelated to the preprocessor expression evaluator (angle include paths, include depth limits, const-discard with FILE, etc.).

---

## Files Changed

- `src/preprocessor.c` — eval_cond_primary (hex + suffix), new eval_cond_ternary, eval_cond_expr update, #if/#elif pre-expansion, remove param_count guard
- `src/version.c` — VERSION_STAGE bump
- 8 new test directories
- 1 new script
