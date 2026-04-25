# Stage-11-07 Kickoff: Integer Literal Typing

## Spec Summary
Stage 11-07 makes integer literal AST nodes carry a type. Unsuffixed
decimal literals default to `int` when they fit in int range; literals
that exceed int range but fit in long become `long`. A new `L`/`l`
suffix forces `long`. Codegen must emit the literal using its type
(long literals materialize into `rax`; int literals into `eax`), and
expression typing must use the literal's type when promoting and
selecting common arithmetic types.

## Out of Scope (per spec)
- unsigned suffixes
- hex/octal/binary literals
- `long long`
- overflow diagnostics beyond a single "too large for supported integer
  types" error

## What changes from prior stages
Currently the lexer reads digits with `atoi` into a 32-bit
`Token.int_value` and the parser builds `AST_INT_LITERAL` from the
raw digit text only. Codegen emits literals as `mov eax, <value>` and
`expr_result_type` returns `TYPE_INT` unconditionally for them. After
this stage:
- The lexer recognizes an optional `L`/`l` suffix and computes the
  literal's `TypeKind`.
- `AST_INT_LITERAL` carries that type on its existing `decl_type`
  field.
- Codegen emits `mov rax, <value>` for long literals and reports the
  literal's type as the expression's result type so common-type
  promotion sees it correctly.

## Planned Changes
1. **Tokenizer** (`src/lexer.c`, `include/token.h`)
   - Add `long long_value` and `TypeKind literal_type` to `Token`.
   - Scan optional `L`/`l` suffix; parse digits with `strtoull`;
     classify as `TYPE_INT` if fits in int and unsuffixed,
     `TYPE_LONG` otherwise; error if too large for long.
   - Keep `value` as the digit text only (no suffix), so codegen can
     emit it directly.
2. **Parser** (`src/parser.c`)
   - In `parse_primary`, copy the token's `literal_type` onto the new
     `AST_INT_LITERAL`'s `decl_type`.
3. **AST** — no struct change; reuse the existing `decl_type` slot.
4. **Codegen** (`src/codegen.c`)
   - For `AST_INT_LITERAL`: emit `mov rax, <value>` when the literal
     is long, else `mov eax, <value>`; set `result_type` accordingly.
   - `expr_result_type` for `AST_INT_LITERAL` returns `node->decl_type`.
5. **Tests** (`test/valid/`)
   - `test_long_literal_suffix__42.c` — `return 42L;`
   - `test_long_literal_lowercase__42.c` — `return 42l;`
   - `test_long_literal_overflows_int__42.c` — value above 2^31.
   - `test_int_plus_long_literal__42.c` — common-type promotion uses
     literal type.
   - `test_long_literal_in_long_var__7.c` — round-trip a value above
     2^31 through a long local.
6. **Documentation**
   - Update `docs/grammar.md`'s `<integer_literal>` production to
     include the optional `L`/`l` suffix.

## Open Spec Questions / Notes
- Spec implies one diagnostic for literals exceeding `long` range —
  implemented as a stderr error and `exit(1)`, matching the project's
  existing error idiom.
- `case <integer_literal>:` dispatch remains 32-bit; long-typed case
  values are out of scope and not exercised by new tests.
