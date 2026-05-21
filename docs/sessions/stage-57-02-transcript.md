# stage-57-02 Transcript: Token Pasting with ##

## Summary

Token pasting (`##` operator) in function-like macro replacement lists concatenates adjacent tokens, discarding surrounding whitespace. When `##` appears between tokens in a replacement list, the preprocessor consumes the `##` token and whitespace, strips trailing whitespace from the left operand's output, and uses the raw (unexpanded) text of the right operand to prevent double expansion.

The feature is implemented in the preprocessor's `substitute_args()` function with validation in the `#define` macro-parsing loop. Invalid uses—`##` in object-like macros, `##` at the start of a replacement list, and `##` at the end of a replacement list—are rejected with diagnostic errors at `#define` time.

## Changes Made

### Step 1: Preprocessor Token-Pasting Logic

- Added `##` detection in `substitute_args()` immediately before the existing `#` stringification check.
- When `##` is found: trailing whitespace is stripped from the output buffer, the `##` token and following whitespace are consumed, and a `paste_next` flag is set.
- When `paste_next` is true for a right-hand parameter, the unexpanded (raw) argument text is used instead of the expanded version to avoid double expansion.

### Step 2: Preprocessor Validation

- Added `##` detection in the macro-definition validation loop (before the existing `#` check).
- Rejects `##` in object-like macros with error "object-like macro cannot contain ##".
- Rejects `##` at the start of a replacement list with error "hash hash at start of replacement list".
- Rejects `##` at the end of a replacement list with error "hash hash at end of replacement list".

### Step 3: Test Cases

- `test_pp_token_paste_join__42.c`: Spec example using `#define JOIN(a, b) a ## b` to paste macro arguments; accesses `int foobar = 42` via `JOIN(foo, bar)` and returns 42.
- `test_pp_token_paste_id_num__42.c`: Identifier + number pasting using `#define VAR(n) var ## n` to access `int var1 = 42` via `VAR(1)` and return 42.
- `test_invalid_143__hash_hash_at_start_of_replacement.c`: Validates rejection of `##` at the start of a replacement list.
- `test_invalid_144__hash_hash_at_end_of_replacement.c`: Validates rejection of `##` at the end of a replacement list.

### Step 4: Documentation

- Updated README.md "Through stage..." line to "stage 57-02 (token pasting with ##)".
- Added preprocessor documentation for token pasting feature: "; token pasting: `##` in function-like macro replacement lists concatenates the adjacent tokens (surrounding whitespace discarded; `##` at the start or end of a replacement list and `##` in object-like macros are rejected at `#define` time)".
- Updated test totals: 635 valid, 201 invalid, 31 integration, 39 print-AST, 99 print-tokens, 21 print-asm; 1026 total.
- No grammar.md update needed (token pasting is a preprocessor transformation, not a grammar rule).

## Final Results

All 1026 tests pass (was 1022 before stage). Suite breakdown:
- valid: 635 (was 633, +2)
- invalid: 201 (was 199, +2)
- integration: 31
- print_ast: 39
- print_tokens: 99
- print_asm: 21

No regressions.

## Session Flow

1. Read spec file, milestone template, and transcript template
2. Examined preprocessor implementation details in `src/preprocessor.c` to understand macro substitution
3. Identified token-pasting insertion point in `substitute_args()` before stringification check
4. Added `##` detection and paste-next flag logic to concatenate tokens with whitespace handling
5. Added validation rules in macro-definition loop to reject invalid `##` uses
6. Reviewed spec example and corrected typo (changed `a && b` to `a ## b`)
7. Created four test cases: two valid (basic pasting, identifier+number), two invalid (start/end position errors)
8. Ran full test suite to verify all 1026 tests pass
9. Updated README.md test counts and added preprocessor feature description
10. Generated milestone and transcript artifacts

