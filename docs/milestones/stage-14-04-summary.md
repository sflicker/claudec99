# Milestone Summary

## Stage-14-04: String Literals as `char *` Expressions — Complete

A string literal expression now decays to `char *` (its `result_type`
becomes `TYPE_POINTER` with `full_type = pointer-to-char`) so it can
initialize and assign to `char *` locals, be indexed through the
resulting pointer, and participate in pointer arithmetic. The
existing pointer-init / pointer-assign / pointer-arith / pointer-
subscript paths in codegen pick up the new typing without any
additional special-casing.

## Behavior gained
- `char *s = "hi"; return s[0];` → 104
- `char *s = "hi"; return s[1];` → 105
- `char *s; s = "hi"; return s[1];` → 105
- `char *s = ""; return s[0];` → 0
- `char *s = "abc"; return s[2];` → 99
- `char *s = "abc"; return *(s + 1);` → 98
- `int *p = "hello";` → rejected with `incompatible pointer type in initializer`

## Codegen changes
- `src/codegen.c`: in `codegen_expression`, the `AST_STRING_LITERAL`
  branch now sets `result_type = TYPE_POINTER` and
  `full_type = type_pointer(type_char())` after pool registration.
- `src/codegen.c`: `codegen_emit_string_pool` derives the byte length
  from `strlen(node->value)` instead of the array `full_type->length`,
  since the literal's `full_type` is rewritten to the decayed
  pointer.

## Test changes
- `test/valid/`: added 6 new tests covering init, assign, indexing,
  empty literal, and pointer-arithmetic dereference.
- `test/invalid/`: added
  `test_invalid_46__incompatible_pointer_type_in_initializer.c`.
- `test/print_asm/`: refreshed the three Stage 14-03 snapshots to
  use a `char *s = "..."` form so they remain valid under the
  stricter return-type check.

## Build & tests
- Build: clean.
- Tests: 379 / 379 pass (238 valid + 45 invalid + 21 print-AST +
  72 print-tokens + 3 print-asm). No regressions.
