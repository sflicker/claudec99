# stage-55-04 Transcript: Unary Preprocessor Operators

## Summary

Stage 55-04 adds unary `!`, `-`, and `+` operators to `#if` and `#elif` preprocessor condition expressions. After a macro identifier is expanded or an integer literal is parsed, these operators are applied (innermost-first for chained forms), and the resulting value is tested zero/nonzero in the usual way.

Examples now supported:
```c
#if !FLAG      // logical not
#if -1         // arithmetic negate → nonzero → true
#if -0         // arithmetic negate → 0 → false
#if +1         // arithmetic identity → nonzero → true
#if !-1        // chained: -(1) = -1, !(-1) = 0 → false
```

## Spec Issues Noted

1. **Tests 4 & 5 incorrect comments** — with `FLAG=1`, both `-FLAG` and `+FLAG` are nonzero, so the true branch (`return 42`) executes and the exit code is 42. The spec comments say "expect status code 1" — this is a copy-paste error. Implemented correctly (exit 42).

2. **`do #endif` artifact** — tests 6–11 in the spec ended with `do #endif` instead of `#endif`. These are copy-paste artifacts. Implemented with plain `#endif`.

## Implementation Plan and Steps

### Step 1: Refactor and extend `src/preprocessor.c`

Added two static helper functions before `preprocess_internal`:

**`eval_cond_primary()`** — handles the three existing primary forms:
- `defined(NAME)` / `defined NAME`
- Integer literal
- Object-like macro identifier (undefined → 0; function-like → error; non-integer expansion → error)

**`eval_cond_expr()`** — handles unary operators:
- Collects leading `!`, `-`, `+` operators into a small array
- Calls `eval_cond_primary()` for the base value
- Applies collected operators in reverse order (innermost-first)
- `!` → logical not; `-` → arithmetic negate; `+` → identity (no-op)

Both helpers take `char *out_data` and `char *spliced_buf` for cleanup on error, matching the existing pattern in `preprocess_internal`.

The `#if` handler's ~60-line inline condition block was replaced with:
```c
while (s[in] == ' ' || s[in] == '\t') in++;
long ifval = eval_cond_expr(s, &in, macros, out.data, spliced);
while (s[in] == ' ' || s[in] == '\t') in++;
if (s[in] != '\n' && s[in] != '\0') { /* extra tokens error */ }
cond_val = (ifval != 0);
```

The `#elif` handler received the identical treatment, eliminating the duplication between the two.

### Step 2: Build and regression test

Build clean (one pre-existing `fread` warning). All 955 prior tests pass.

### Step 3: Add 11 new test files to `test/valid/`

| File | Condition | Expected |
|---|---|---|
| `test_pp_if_unary_not_macro_false__42.c` | `!FLAG` (FLAG=0) | 42 |
| `test_pp_if_unary_neg_macro_zero__1.c` | `-FLAG` (FLAG=0) | 1 |
| `test_pp_if_unary_not_macro_true__1.c` | `!FLAG` (FLAG=1) | 1 |
| `test_pp_if_unary_neg_macro_nonzero__42.c` | `-FLAG` (FLAG=1) | 42 |
| `test_pp_if_unary_plus_macro_nonzero__42.c` | `+FLAG` (FLAG=1) | 42 |
| `test_pp_if_unary_neg_literal__42.c` | `-1` | 42 |
| `test_pp_if_unary_plus_literal__42.c` | `+1` | 42 |
| `test_pp_if_unary_neg_zero__1.c` | `-0` | 1 |
| `test_pp_if_unary_not_zero__42.c` | `!0` | 42 |
| `test_pp_if_unary_not_one__1.c` | `!1` | 1 |
| `test_pp_if_unary_not_neg_one__1.c` | `!-1` (chained) | 1 |

### Step 4: Documentation

- `docs/grammar.md`: updated `<if-condition>` to expand through `<pp-unary-expression>`, `<pp-unary-op>`, and `<pp-primary>`.
- `README.md`: updated "Through stage" line, preprocessor feature bullet (added unary operators), "Not yet supported" section (binary arithmetic still out, unary now in), and test totals.

## Final Results

All 966 tests pass:
- Valid: 588 (+11)
- Invalid: 192
- Integration: 27
- Print-AST: 39
- Print-tokens: 99
- Print-asm: 21

No regressions. Full test suite clean.

## Files Changed

- `src/preprocessor.c`
- `test/valid/test_pp_if_unary_not_macro_false__42.c` (new)
- `test/valid/test_pp_if_unary_neg_macro_zero__1.c` (new)
- `test/valid/test_pp_if_unary_not_macro_true__1.c` (new)
- `test/valid/test_pp_if_unary_neg_macro_nonzero__42.c` (new)
- `test/valid/test_pp_if_unary_plus_macro_nonzero__42.c` (new)
- `test/valid/test_pp_if_unary_neg_literal__42.c` (new)
- `test/valid/test_pp_if_unary_plus_literal__42.c` (new)
- `test/valid/test_pp_if_unary_neg_zero__1.c` (new)
- `test/valid/test_pp_if_unary_not_zero__42.c` (new)
- `test/valid/test_pp_if_unary_not_one__1.c` (new)
- `test/valid/test_pp_if_unary_not_neg_one__1.c` (new)
- `docs/grammar.md`
- `docs/kickoffs/stage-55-04-kickoff.md` (generated)
- `docs/milestones/stage-55-04-milestone.md` (generated)
- `docs/sessions/stage-55-04-transcript.md` (this file)
- `README.md`
