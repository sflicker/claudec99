# Stage 56-01 Kickoff: `#error` Directive

## Spec Summary

Stage 56-01 adds support for the `#error` preprocessing directive. This directive allows compilation to fail with a user-provided error message during preprocessing, but only when the directive appears in an active conditional region. `#error` directives inside inactive conditional regions (e.g., within `#if 0` blocks) are silently skipped.

**Behavior:**
- In active regions: emit a fatal preprocessor error with the message text and halt compilation
- In inactive regions: skip silently (do not emit any error)
- Message text: all tokens after `#error` until end of line

## Tokenizer Changes

**None.** The tokenizer does not parse `#error` directives; they are handled at the preprocessor level.

## Parser Changes

**None.** The `#error` directive is purely a preprocessor feature and does not affect the parser.

## AST Changes

**None.** No new AST nodes are required.

## Code Generation Changes

**None.** The `#error` directive halts compilation during preprocessing; no code is generated.

## Preprocessor Changes

### Summary

Add `#error` directive handling to `src/preprocessor.c`. The handler is placed after the inactive-region skip guard (around line 1071) and before the unsupported-directive error. This ensures:
- `#error` in inactive conditional regions passes the skip guard and is ignored
- `#error` in active regions reaches the handler, emits a fatal diagnostic, and exits

### Implementation Steps

1. **Locate the preprocessor directive dispatch** in `src/preprocessor.c` (around line 1050-1100)
2. **Identify the inactive-region skip guard** that prevents processing directives in `#if 0` / `#else` blocks
3. **Add `#error` handler** after the skip guard:
   - Extract message text (all tokens after `#error` until newline)
   - Call `fatal_error()` or similar with the message
   - Exit compilation
4. **Verify placement** — handler should come before the generic "unsupported directive" error

## Test Plan

Create two test files:

### 1. Valid Test: `test/valid/test_pp_error_in_if_0__42.c`

Tests that `#error` inside `#if 0` is silently skipped and does not halt compilation.

```c
#if 0
#error this should be skipped
#endif

int main() {
    return 42;
}
```

**Expected:** Compilation succeeds; program returns 42.

### 2. Invalid Test: `test/invalid/test_pp_error_directive__error_directive.c`

Tests that a bare `#error` in an active region halts compilation with the error message.

```c
#error unsupported platform

int main() {
    return 0;
}
```

**Expected:** Compilation fails with a fatal error containing "unsupported platform".

## Known Issues in Spec

### Garbled Prose

The spec contains the phrase: "so this should be this test should be in the invalid category" — a doubled phrasing, but intent is clear: the second test belongs in the invalid category.

### Invalid C in Valid Test

The original valid test in the spec contained an `if` statement without a condition. This is invalid C syntax. The corrected version uses a plain `return 42;` statement, which is valid and achieves the intended behavior (returns 42 when the `#error` is skipped).

## Implementation Order

1. Preprocessor handler in `src/preprocessor.c`
2. Test files in `test/valid/` and `test/invalid/`
3. Update `README.md` with stage completion
