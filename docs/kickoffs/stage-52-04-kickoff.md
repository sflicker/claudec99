# Stage 52-04 Kickoff

## Spec Summary

Add support for `#elif <integer-constant>` in the preprocessor. This stage implements conditional branches that execute only if previous branches were not taken and the elif condition evaluates to true.

Key rules:
- `0` evaluates to false; nonzero evaluates to true
- First true branch wins; later branches are skipped once any branch is taken
- Supports multiple `#elif` branches in a single conditional chain
- `#elif` can follow `#if`, `#ifdef`, or `#ifndef`
- Whitespace allowed between `#` and `elif`, and between `elif` and the integer
- Only a single integer literal is valid — no expressions, identifiers, or `defined()`

## Derived Stage Values

| Component | Change Required |
|-----------|-----------------|
| Tokenizer | None |
| Parser | None |
| AST | None |
| Code generation | None |
| Preprocessor | Yes — add `#elif` handler, `branch_taken` tracking |

## Planned Changes

### Preprocessor (`src/preprocessor.c`)
- Add `branch_taken` field to `CondFrame` struct to track whether any branch in the conditional chain has been selected
- Initialize `branch_taken = false` in `#ifdef`, `#ifndef`, and `#if` handlers
- Add `#elif <integer>` handler that:
  - Rejects `#elif` appearing without an open conditional
  - Rejects `#elif` appearing after `#else`
  - Evaluates the integer literal (0 = false, nonzero = true)
  - Activates the block if `!branch_taken && condition_true`
  - Sets `branch_taken = true` when a branch is taken
- Update `#else` handler to use `!top->branch_taken` instead of `!top->emitting` to prevent stale emitting state from a prior `#elif` from incorrectly activating the `#else` block

## Test Plan

**Valid tests (4):**
1. `test/valid/test_pp_elif_after_ifndef_true__42.c` — `#ifndef` (false) + `#elif 1` → return 42
2. `test/valid/test_pp_elif_after_ifndef_nonzero__42.c` — `#ifndef` (false) + `#elif 114` → return 42
3. `test/valid/test_pp_elif_skipped_after_ifdef__1.c` — `#ifdef` (true) + `#elif 1` → return 1 (elif skipped)
4. `test/valid/test_pp_elif_multi_branch__2.c` — `#ifndef` (false) + `#elif 0` + `#elif 1` → return 2

**Invalid tests (2):**
1. `test/invalid/test_pp_elif_without_conditional__elif_without_conditional.c` — `#elif` with no open conditional
2. `test/invalid/test_pp_elif_after_else__elif_after_else.c` — `#elif` appearing after `#else`

## Implementation Order

1. Preprocessor changes in `src/preprocessor.c`
2. Valid tests (4)
3. Invalid tests (2)
4. Update `docs/grammar.md` to document `#elif` syntax
5. Update `README.md` to mention stage-52-04

## Ambiguities and Decisions

- Test 4 in the spec has typos: `int main() [` uses `[` instead of `{`, and has no closing `}` before `#endif`. The intent is clear (expected exit code 2); both are corrected in the actual test file.
- No spec ambiguity on semantics — `branch_taken` tracks "has any branch been selected" to implement first-true-wins correctly across multiple elif chains, ensuring that once a branch executes, subsequent `#elif` and `#else` blocks are skipped.
