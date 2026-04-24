# Stage-11-05-03 Milestone: Conversions at the Function Boundary

Completed the integer type system across function boundaries. Value
conversions (widen via sign-extend, or truncate) are now emitted at
every point where a value crosses the function interface: call-site
arguments, the function prologue's parameter stores, and each
`return` statement.

## What was completed
- `CodeGen` (include/codegen.h) now carries `current_return_type`
  (set at the top of every function) and `tu_root` (set in
  `codegen_translation_unit`) so call sites can look up a callee's
  declared parameter types and `return` statements can reach the
  enclosing function's declared return type.
- Added `emit_convert(cg, src, dst)` in src/codegen.c, shared by call
  sites and `return` statements: emits `movsxd` to widen to long,
  `movsx eax, ax`/`movsx eax, al` to narrow to short/char, and no
  instruction for the same size or for a long→int narrowing (which is
  implicit since `eax` already holds the low 32 bits of `rax`).
- Added `codegen_find_function_decl` so call sites can resolve the
  callee's `AST_FUNCTION_DECL` without adding a new AST field.
- `AST_FUNCTION_CALL` now converts each evaluated argument to the
  callee's declared parameter type (via its `AST_PARAM` child's
  `decl_type`) before pushing.
- The function prologue now sizes each parameter's stack slot by its
  declared type (1/2/4/8 bytes) and stores from the width-matched
  sub-register (e.g. `dil`/`di`/`edi`/`rdi`). Frame-size estimate
  widened from 4 to 8 bytes per parameter for the new sizes.
- `AST_RETURN_STATEMENT` now invokes `emit_convert` with the
  expression's `result_type` and the function's
  `current_return_type` before placing the value in the return
  register.
- Added six new valid tests exactly covering the spec's testing
  matrix: argument narrowing, argument widening, return widening,
  return narrowing, mixed parameter types, and a call whose argument
  is an arithmetic expression combined with a return-site widening.

## Scope limits (per spec)
No grammar, tokenizer, parser, AST-struct, or AST-pretty-printer
changes. No strict type errors introduced. No casts, unsigned types,
or floating point.

## Test results
All 199 tests pass (172 valid + 14 invalid + 13 print_ast),
including the 6 new tests. No regressions.
