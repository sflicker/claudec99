# stage-14-04 Transcript: String Literals as `char *` Expressions

## Summary

Stage 14-04 promotes a string literal's expression-result type from
the opaque `TYPE_LONG` address used in Stage 14-03 to a properly
decayed `char *` (`TYPE_POINTER` with `full_type` chain
`pointer -> char`). The change activates the existing pointer-init,
pointer-assign, pointer-arithmetic, and pointer-subscript paths
without introducing any new special cases for string literals.

This enables `char *s = "hi";` initialization, `s = "hi";`
assignment, `s[i]` indexing, and `*(s + 1)` pointer arithmetic.
Cross-base initializers like `int *p = "hello";` are rejected by
the existing `pointer_types_equal` check at the declaration site.

The static-data emitter in `codegen_emit_string_pool` was switched
from reading the array length off the AST node's `full_type`
(formerly `char[N+1]`) to using `strlen(node->value)`, because the
node's `full_type` is rewritten to the decayed pointer once the
literal is emitted as an expression. The lexer already rejects
escape sequences, so the payload has no embedded NULs and `strlen`
is exact.

## Changes Made

### Step 1: Code Generator

- In `codegen_expression`, the `AST_STRING_LITERAL` branch now sets
  `node->result_type = TYPE_POINTER` and
  `node->full_type = type_pointer(type_char())` after pool
  registration and the `lea rax, [rel Lstr<N>]` emission.
- In `codegen_emit_string_pool`, the per-entry `byte_len` is now
  computed as `(int)strlen(s->value)` instead of
  `s->full_type->length - 1`.

### Step 2: Tests

- Added 6 new valid tests in `test/valid/`:
  - `test_string_literal_init_index_0__104.c`
  - `test_string_literal_init_index_1__105.c`
  - `test_string_literal_assign_then_index__105.c`
  - `test_string_literal_empty__0.c`
  - `test_string_literal_init_index_2__99.c`
  - `test_string_literal_pointer_arith_deref__98.c`
- Added 1 invalid test in `test/invalid/`:
  - `test_invalid_46__incompatible_pointer_type_in_initializer.c`
    (covers `int *p = "hello";`).
- Refreshed the three Stage 14-03 print_asm snapshots so they no
  longer return a `char *` from `int main()`. Each now uses
  `char *s = "..."; return 0;` and the `.expected` files were
  regenerated.

## Final Results

All 379 tests pass (238 valid + 45 invalid + 21 print-AST + 72
print-tokens + 3 print-asm). No regressions.

## Session Flow

1. Read the Stage 14-04 spec and the Stage 14-03 milestone.
2. Reviewed `src/codegen.c` (string-literal expression path,
   declaration init path, pointer arithmetic, return-type check)
   and `src/parser.c` (string-literal AST construction).
3. Identified that the only required change was promoting the
   string-literal expression result to `char *` and adjusting the
   pool emitter to recover byte length from the payload.
4. Saved kickoff and confirmed plan with the user.
5. Implemented the two `codegen.c` edits and rebuilt cleanly.
6. Added 6 valid tests, 1 invalid test, and refreshed the three
   Stage 14-03 print_asm snapshots (sources and `.expected` files).
7. Ran the full aggregate test suite — 379/379 pass.
8. Wrote the milestone summary and this transcript.

## Notes

- The Stage 14-03 print_asm snapshots had to be refreshed because
  `int main() { return "hi"; }` now triggers the strict
  pointer-vs-non-pointer return-type check from Stage 12-05. The
  refreshed sources still exercise `.rodata` emission and the
  `lea rax, [rel Lstr<N>]` materialization but route the literal
  through a `char *` local instead of the return path.
- The spec's invalid test code block was missing its closing brace;
  treated as a typo and added in the test file.
