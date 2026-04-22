# Stage-11-03 Milestone: Integer Promotions and Common-Type Arithmetic

Added semantic and codegen support for integer promotions and
common-type selection in arithmetic expressions over `char`, `short`,
`int`, and `long`.

## What was completed
- `ASTNode` carries a `TypeKind result_type` populated during codegen.
- New codegen helpers: `promote_kind`, `common_arith_kind`,
  `type_kind_from_size`, and `expr_result_type` (which also writes
  `result_type` onto the node).
- `emit_load_local` now loads a full `long` into `rax` (was a 32-bit
  low-word load).
- `emit_store_local` gained a `src_is_long` parameter; size-8 stores
  only sign-extend `eax → rax` when the source was an int-sized value.
- Binary `+ - * /` evaluates in the common type. When the common type
  is `long`, int-sized operands are widened with `movsxd rax, eax`
  before the op and 64-bit forms are emitted (`add/sub/imul rax, rcx`,
  `cqo` + `idiv rcx`).
- Unary `-` emits `neg rax` for `long` operands, `neg eax` otherwise.
- Assignment and declaration-with-initializer use the RHS expression's
  `result_type` to drive the new store path.
- Seven new valid tests covering long×int, long−int, int/long,
  char+char → int, long unary `-`, long = int+int, and short+long.

## Scope limits (per spec)
Unsigned types, casts, literal suffixes, `long long`, comparison /
logical / bitwise type awareness, and function parameter/return-type
widening are out of scope. Integer literals still default to `int`.

## Test results
All 175 tests pass (149 valid + 14 invalid + 12 print_ast). No
regressions.
