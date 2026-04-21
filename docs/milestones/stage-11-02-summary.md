# Stage-11-02 Milestone: Integer Type Codegen

Extended codegen so local variables declared `char`, `short`, `int`,
and `long` are stored at their proper size and alignment (1/2/4/8
bytes) instead of always being emitted as 32-bit ints.

## What was completed

- `LocalVar` records the storage size of each declared local.
- New helpers `type_kind_bytes`, `emit_load_local`, and
  `emit_store_local` centralize width-aware load/store emission.
- `codegen_add_var` now takes a size, advances `stack_offset` by that
  size, and aligns up to a natural boundary.
- `AST_VAR_REF` and `AST_ASSIGNMENT` emit size-specific mnemonics
  (`movsx`/`mov` for loads; `al`/`ax`/`eax`/`rax` for stores with long
  stores sign-extending `eax` → `rax`).
- `AST_PREFIX_INC_DEC` and `AST_POSTFIX_INC_DEC` use the same size-aware
  load/store path; postfix result restored from a saved copy after the
  store (the long-store path clobbers `rax`).
- `AST_DECLARATION` stores its initializer through the truncating store
  helper so `char a = 273;` correctly yields 17.
- Stack-size computation replaced with a conservative 8-bytes-per-
  declaration walk, keeping the 16-byte stack-alignment requirement.
- Parameters remain `int`-only (4 bytes) per the stage spec.
- Six new valid tests covering truncation into char/short, postfix
  inc/prefix dec on char/long, assignment without initializer, and
  mixed char/short/long arithmetic with compound assignment.

## Scope limits (per spec)

Integer promotions, usual arithmetic conversions, unsigned behavior,
mixed-type expression rules, casts, parameter-passing changes, and
return-type widening/narrowing are intentionally not handled in this
stage. Arithmetic continues to execute in 32-bit `eax`; `long` loads
read the low 32 bits; `long` stores sign-extend `eax` to the full
8-byte slot.

## Test results

160 tests pass (134 valid + 14 invalid + 12 print_ast). No regressions.
