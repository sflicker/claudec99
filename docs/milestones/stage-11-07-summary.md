# Stage-11-07 Milestone: Integer Literal Typing

Integer literals now carry a type. Unsuffixed decimal literals are
typed `int` when they fit in 32-bit signed range and `long` otherwise;
the new `L`/`l` suffix forces `long`. Codegen emits each literal with
an instruction sized for its type, and the literal's type participates
in expression promotion and common-type selection.

## What was completed
- Tokenizer (src/lexer.c, include/token.h): integer literal scanning
  optionally consumes a single `L` or `l` suffix; the digits are
  parsed with `strtoul` and classified as `TYPE_INT` (unsuffixed and
  fits in `INT_MAX`) or `TYPE_LONG` (suffixed, or value above
  `INT_MAX`); values exceeding `LONG_MAX` are rejected with
  "integer literal '...' too large for supported integer types". The
  `Token` struct carries the parsed value as `long long_value` and the
  type as `literal_type` (replacing the unused `int int_value`).
- Parser (src/parser.c): `parse_primary` copies the token's
  `literal_type` onto the new `AST_INT_LITERAL`'s `decl_type`.
- AST: no struct change; the existing `decl_type` slot stores the
  literal's type, mirroring how casts and declarations carry their
  declared type.
- Code generator (src/codegen.c): `AST_INT_LITERAL` emits
  `mov rax, <value>` when the literal is long and `mov eax, <value>`
  otherwise, and reports the matching `result_type`. `expr_result_type`
  for `AST_INT_LITERAL` returns the stored `decl_type` so binary-op
  common-type analysis sees `long` literals correctly.
- Five new valid tests covering the spec examples and behavior:
  `42L` and `42l` suffix forms, an unsuffixed value above 2^31 forced
  to long, an `int + long` literal common-type promotion, and a long
  literal stored in a long local and rounded back through an int cast.
- `docs/grammar.md` updated: `<integer_literal>` now reads
  `[0-9]+ [Ll]?`.

## Scope limits (per spec)
No unsigned suffixes, no hex/octal/binary literals, no `long long`,
and only a single overflow diagnostic for values that exceed long
range. `case <integer_literal>:` dispatch remains 32-bit; long-typed
case values are out of scope.

## Test results
All 210 tests pass (183 valid + 14 invalid + 13 print_ast), including
the 5 new literal-typing tests. No regressions.
