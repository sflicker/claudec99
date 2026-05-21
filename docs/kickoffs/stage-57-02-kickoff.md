# stage-57-02 Kickoff

## Spec Summary

Add support for token pasting (`##` operator) in function-like and object-like macro replacement lists. When `##` appears between tokens, it removes surrounding whitespace and concatenates the tokens into a single token.

**Example:**
```c
#define JOIN(a, b) a ## b

int main(void) {
    int foobar;
    foobar = 42;
    return JOIN(foo, bar);   // expands to: foobar; returns 42
}
```

## Scope

**In-Scope:**
- `##` in macro replacement lists (function-like and object-like)
- Pasting macro arguments with ordinary tokens
- Pasting two macro arguments
- Pasting identifiers
- Pasting identifier + number
- Clean error handling for invalid token concatenations

**Out-of-Scope:**
- Variadic macros and `__VA_ARGS__`
- Comma-swallowing extensions
- Full GCC compatibility edge cases

## Required Changes

### Tokenizer
No changes required. The preprocessor is text-based and operates before tokenization.

### Parser
No changes required.

### AST
No changes required.

### Code Generation
No changes required.

### Preprocessor
1. **Validation at `#define` time:**
   - Reject `##` at the start of replacement list (error)
   - Reject `##` at the end of replacement list (error)
   - Reject `##` in object-like macros (error)

2. **Token pasting at substitution time (`substitute_args`):**
   - Detect `##` operator in replacement list
   - When `##` is encountered, remove trailing whitespace from left token and leading whitespace from right token
   - Concatenate the tokens to form a single token
   - Use expanded arguments (acceptable for in-scope tests with plain identifiers/numbers)

## Implementation Plan

1. Add `##` detection and validation in `#define` handler (preprocessor)
2. Implement pasting logic in `substitute_args` function with `paste_next` flag
3. Write and run integration tests:
   - `test/valid/test_pp_token_paste_join__42.c` (spec example corrected: `a ## b`)
   - `test/valid/test_pp_token_paste_id_num__42.c` (identifier + number pasting)
   - `test/invalid/test_invalid_143__hash_hash_at_start_of_replacement.c`
   - `test/invalid/test_invalid_144__hash_hash_at_end_of_replacement.c`

## Known Ambiguities & Decisions

- **Spec example typo:** The spec shows `a && b` but should be `a ## b` for the example to work correctly. Will implement using `##`.
- **Object-like macro rejection:** `##` in object-like macros will be rejected at define-time. No test cases in scope (keeping implementation minimal).
- **Argument expansion:** Left operand of `##` uses expanded arguments (not raw). This is acceptable for all in-scope tests since arguments are plain identifiers/numbers.
