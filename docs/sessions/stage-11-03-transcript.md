# stage-11-03 Transcript: Integer Promotions and Common-Type Handling

## Summary

Stage 11-03 extends the compiler with integer promotions and
common-type selection for arithmetic expressions over `char`, `short`,
`int`, and `long`. Expressions now carry a resulting type, `char` and
`short` operands promote to `int` in arithmetic, binary `+ - * /`
select a common type that is `long` if either operand is `long` and
`int` otherwise, and unary `-` respects the promoted operand type.

The emitted assembly branches between 32-bit (`eax`) and 64-bit
(`rax`) forms based on the common type. Long operands are loaded
with `mov rax, [...]`; int-sized operands are widened via
`movsxd rax, eax` before participating in a 64-bit operation.
Assignment and declaration-with-initializer continue to use the
existing size-aware truncating store, now parameterized on whether
the source value is already in the full `rax`.

Unsigned types, casts, literal suffixes, `long long`, parameter and
return-type widening, and type-aware comparison / logical / bitwise
operators remain out of scope.

## Changes Made

### Step 1: AST

- Added `TypeKind result_type` field to `ASTNode` in `include/ast.h`.

### Step 2: Code Generator

- Added `promote_kind`, `common_arith_kind`, `type_kind_from_size`,
  and `expr_result_type` helpers in `src/codegen.c`.
- Changed `emit_load_local` so size 8 emits `mov rax, [rbp - offset]`
  (was `mov eax, [rbp - offset]`).
- Added `src_is_long` parameter to `emit_store_local`; size-8 stores
  skip the `movsxd rax, eax` sign-extension when the source is
  already a long.
- `AST_INT_LITERAL`, `AST_VAR_REF`, `AST_ASSIGNMENT`, `AST_UNARY_OP`,
  `AST_PREFIX_INC_DEC`, `AST_POSTFIX_INC_DEC`, `AST_FUNCTION_CALL`,
  and `AST_BINARY_OP` write `result_type` onto the node.
- `AST_UNARY_OP` emits `neg rax` when the promoted operand type is
  `long`, `neg eax` otherwise. Unary `+` propagates the promoted
  type as a no-op.
- `AST_BINARY_OP` for `+ - * /` computes the common type, sign-extends
  int-sized sides with `movsxd rax, eax` before the op when the
  common type is `long`, and emits 64-bit forms
  (`add/sub/imul rax, rcx`, `cqo` + `idiv rcx`) for long arithmetic.
- Comparison, `&&`, `||`, and inc/dec paths continue to emit 32-bit
  code and report `TYPE_INT` as their result type.
- `AST_DECLARATION` and `AST_ASSIGNMENT` pass the RHS expression's
  `result_type` to `emit_store_local`.

### Step 3: Tests

- Added `test_long_times_int__50.c`
- Added `test_long_minus_int__40.c`
- Added `test_int_div_long__5.c`
- Added `test_char_promote_to_int__128.c`
- Added `test_long_unary_neg__42.c`
- Added `test_long_assign_from_int_sum__30.c`
- Added `test_short_plus_long__42.c`

## Final Results

All 175 tests pass (149 valid + 14 invalid + 12 print_ast). No regressions.

## Session Flow

1. Read spec and supporting files.
2. Reviewed existing codegen, parser, AST, and relevant tests.
3. Produced kickoff summary, called out the stray-apostrophe typo,
   and proposed planned changes.
4. Added `result_type` field to `ASTNode`.
5. Added type helpers and refactored codegen for promotions and
   common-type arithmetic; updated load/store helpers.
6. Updated assignment/declaration store sites to pass the
   `src_is_long` flag.
7. Built the compiler and ran all three test suites (valid, invalid,
   print_ast).
8. Added seven new valid tests targeting promoted and long-result
   arithmetic and assignment conversion.
9. Inspected emitted assembly for a char-promotion case, a
   long × int case, and a long unary-neg case to verify the chosen
   mnemonics.
10. Wrote kickoff, milestone, and transcript artifacts.
