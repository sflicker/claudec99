# Stage-12-04 Kickoff: Pointer Parameters and Arguments

## Spec Summary
Stage 12-04 extends pointer support across the function-call boundary.
Functions may declare pointer-typed parameters (e.g. `int *p`); callers
may pass pointer-valued expressions (variable references, address-of,
dereference) as arguments. Pointer parameters are stored as 8-byte
locals so the existing dereference / assign-through-pointer machinery
(added in 12-02 and 12-03) works inside the callee. The compiler
rejects integer/pointer mismatches and mismatched pointer base types
at the call site.

## What changes from Stage 12-03
- Grammar: `<parameter_declaration>` is widened from
  `<integer_type> <identifier>` to `<type> <identifier>`, where
  `<type> ::= <integer_type> { "*" }` (already in use for local
  declarations).
- Parser: `parse_parameter_declaration` accepts `*` after the integer
  base type, building a Type chain. The resulting AST_PARAM is
  annotated with `decl_type = TYPE_POINTER` and a `full_type` chain.
- Codegen: the parameter prologue uses 8-byte slots for pointer
  parameters and records `full_type` on the local so deref/store
  paths can recover the base type. The call-site argument handling
  adds pointer/integer mismatch and base-type-mismatch checks.
- AST pretty-printer: AST_PARAM renders pointer types via the
  existing `ast_print_type` helper, mirroring AST_DECLARATION.

## Spec issues / clarifications
1. **Bug in "Nested pointer parameter" test** — the inner function
   references `p` (undefined). The parameter is `pp`. Implementation
   uses `return **pp;`.
2. **Bug in "Invalid: Integer passed to pointer parameter" test** —
   `int main(*)` is malformed and the closing `}` is missing.
   Implementation uses a clean `int main() { ... }` body.
3. **Typo** — `<identifer>` in the grammar block; should be
   `<identifier>`.
4. **Formatting** — `= **Invalid:: Mismatched pointer type**` has a
   stray `=` and a double colon. Cosmetic only.
5. The spec does not specify exact error wording. Error fragments are
   chosen consistently with existing invalid-test naming style.

## Planned Changes

1. **Tokenizer** — no changes.
2. **Parser** (`src/parser.c`)
   - Replace `parse_parameter_declaration` with a body that consumes
     the integer base type, then zero or more `*` tokens, building a
     `Type` chain. AST_PARAM gets `decl_type = TYPE_POINTER` and
     `full_type` when at least one `*` is consumed.
3. **AST** — no new node types. Existing `full_type` field on
   AST_PARAM is now populated for pointer parameters.
4. **AST pretty-printer** (`src/ast_pretty_printer.c`)
   - In the AST_PARAM branch, render the type via `ast_print_type`
     when `decl_type == TYPE_POINTER`, mirroring the AST_DECLARATION
     form.
5. **Code generator** (`src/codegen.c`)
   - In the parameter prologue, pass `node->children[i]->full_type`
     (instead of `NULL`) to `codegen_add_var` so the local carries the
     pointer chain.
   - In the function-call argument loop, when the parameter or
     argument is a pointer, replace the integer-style `emit_convert`
     with strict type checks: param-pointer + arg-non-pointer →
     error; param-non-pointer + arg-pointer → error; both pointer but
     base chains differ → error. No conversion is emitted on the
     matching pointer path (the address is already in `rax`).
6. **Tests**
   - Valid:
     - `test_pointer_param_read__7.c` — `int read(int *p) { return *p; }`
     - `test_pointer_param_write__9.c` — write through pointer param.
     - `test_nested_pointer_param__11.c` — `int read2(int **pp) { return **pp; }`.
   - Invalid:
     - `test_invalid_20__expected_pointer_argument.c` — int → pointer param.
     - `test_invalid_21__expected_integer_argument.c` — pointer → int param.
     - `test_invalid_22__incompatible_pointer.c` — `char *` → `int *`.
   - Print-AST: add a new test that renders a function with a
     pointer parameter.
7. **Documentation**
   - Update `docs/grammar.md`: change `<parameter_declaration>` to
     `<type> <identifier>`.
8. **Commit** at the end with a concise message.
