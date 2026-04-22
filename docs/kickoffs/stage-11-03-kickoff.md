# Stage-11-03 Kickoff: Integer Promotions and Common-Type Handling

## Spec Summary
Stage 11-03 adds integer promotions and common-type selection for
expressions mixing `char`, `short`, `int`, and `long`. Expressions must
track a resulting type, `char`/`short` operands promote to `int`,
binary arithmetic selects a common type (`long` if either side is
`long`, else `int`), and assignment converts the RHS to the LHS
declared type on store using the stage-11-02 truncating-store path.

## What Must Change From Stage 11-02
- Expression AST nodes carry a resulting `TypeKind`.
- `AST_VAR_REF` for `long` variables loads the full 8 bytes into `rax`
  (was low 32 bits only).
- Binary arithmetic `+ - * /` evaluates in 32-bit when the common type
  is `int` and in 64-bit when the common type is `long`; mixed-width
  operands are sign-extended with `movsxd rax, eax` before the op.
- Unary `-` uses `neg rax` for long operands, `neg eax` otherwise.
- `emit_store_local` gains a `src_is_long` parameter so size-8 stores
  know whether to sign-extend `eax` into `rax` or use `rax` directly.

## Out of Scope
Unsigned types, casts, literal suffixes, `long long`, comparisons /
logical / bitwise type awareness, parameter and return-type widening,
narrowing diagnostics, overflow. Integer literals still default to
`int`.

## Spec Notes / Ambiguity
- Spec rule 2 has a stray apostrophe (`int op long' -> long`); read
  as `int op long -> long`.
- Requirement 3 allows tracking in the AST "or semantic layer"; a
  `result_type` field was added to `ASTNode` and populated during
  codegen.

## Planned Changes
- **AST**: `TypeKind result_type` field on `ASTNode`.
- **Codegen**: type helpers (`promote_kind`, `common_arith_kind`,
  `type_kind_from_size`, `expr_result_type`); long-aware load/store;
  long-aware `neg`; promotion/common-type path in binary arithmetic.
- **Tests**: new valid tests covering long×int, long−int, int/long,
  char+char promoted to int, long unary `-`, long = int+int, and
  short+long.
- **Grammar**: none (spec states no grammar changes).
