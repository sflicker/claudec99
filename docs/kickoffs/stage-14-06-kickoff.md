# Stage-14-06 Kickoff

## Spec
`docs/stages/ClaudeC99-spec-stage-14-06-array-initialization-from-string-literals.md`

## Stage Goal
Allow local `char` arrays to be initialized from a string literal:

- Explicit size: `char s[N] = "..."` — N must be ≥ literal_byte_length+1;
  missing bytes are zero-filled.
- Inferred size: `char s[] = "..."` — array length becomes
  literal_byte_length+1.

The string's payload bytes (decoded by Stage 14-05) plus a trailing
NUL are copied into the local stack slot.

## Delta from Stage 14-05
Stage 14-05 left arrays unassignable and rejected any `=` initializer
on an array declaration with `error: array initializers not
supported`. Stage 14-06 carves out a single supported initializer
form — a string literal targeting a `char` array — both with
explicit and inferred size, and adds the four new error categories
spelled out in the spec.

## Ambiguities / Spec Issues Flagged
- Bytes-per-literal annotation typo: spec says `"hi"` is
  `105, 106, 0`. `'h'` is 104 and `'i'` is 105; the later test
  expectations (`return s[0] // expect 104`) confirm conventional
  ASCII. Implementation follows the conventional values.
- Missing identifier in spec valid test: spec reads
  `char[5] = "hi";`. Treated as a typo; implemented as
  `char s[5] = "hi";`.
- Missing opening quote in inferred-size example: spec reads
  `` ` char s[] = A\0B"; ` ``. Treated as `char s[] = "A\0B";` per
  the surrounding text.
- Grammar interaction: the new grammar
  `<declaration> ::= <type> <identifier> [ "[" [ <integer_literal> ] "]" ] [ "=" <expression> ] ";"`
  makes `<integer_literal>` inside `[]` optional only when paired
  with a string-literal initializer. This is a semantic restriction
  (not expressible in EBNF) and is enforced in the parser per the
  spec restriction.

## Planned Changes

### Tokenizer (`src/lexer.c`)
- No changes. Existing `[`, `]`, and string-literal tokens suffice.

### Parser (`src/parser.c`)
- In the array-suffix branch of `parse_statement`'s declaration
  handler:
  - Accept an empty `[]` and defer length resolution to the
    initializer.
  - Replace the unconditional
    `error: array initializers not supported` with a switch on the
    initializer form:
    - `=` followed by a string literal: build the declaration with
      the literal as a child. Element type must be `char`;
      otherwise raise
      `error: string initializer only supported for char arrays`.
    - `=` followed by anything else for an empty `[]`:
      `error: omitted array size requires string literal initializer`.
    - `=` followed by anything else for an explicit `[N]`: keep
      `error: array initializers not supported`.
  - Empty `[]` without initializer:
    `error: array size required unless initialized from string literal`.
  - Empty `[]` with string-literal initializer: set
    `length = literal_byte_length + 1` and build the array Type
    accordingly.
  - Explicit `[N]` with string-literal initializer: enforce
    `N >= literal_byte_length + 1`; otherwise raise
    `error: array too small for string literal initializer`.

### AST
- No structural changes. `AST_DECLARATION` for `TYPE_ARRAY` will
  optionally carry one child (`AST_STRING_LITERAL`) when a
  string-literal initializer is present.

### Code Generator (`src/codegen.c`)
- In `codegen_statement` / `AST_DECLARATION`, the `TYPE_ARRAY`
  branch:
  - Allocate the slot via the existing `codegen_add_var` call.
  - When the array carries a string-literal initializer child:
    - Pool the string literal so it ends up in `.rodata`
      (matching how string-literal expressions are pooled today).
    - Emit byte stores for each payload byte into
      `[rbp - offset + i]`, then zero-fill bytes from
      `byte_length+1` up to the array's total size.

### AST Pretty Printer (`src/ast_pretty_printer.c`)
- No special handling needed. The string-literal child is visited
  by the generic child-walk and renders as a `StringLiteral:` line
  under the declaration. Inferred-size declarations will display
  the inferred length as the bracket count.

### Tests
Valid (`test/valid/`), one per spec example:
- `test_string_array_init_inferred_index_0__104.c`
- `test_string_array_init_inferred_index_1__105.c`
- `test_string_array_init_inferred_index_2__0.c`
- `test_string_array_init_explicit_5_index_4__0.c`
- `test_string_array_init_inferred_escape_n_index_1__10.c`
- `test_string_array_init_inferred_escape_null_index_2__66.c`

Invalid (`test/invalid/`):
- `test_invalid_48__array_too_small_for_string_literal_initializer.c`
- `test_invalid_49__string_initializer_only_supported_for_char_arrays.c`
- `test_invalid_50__array_size_required_unless_initialized_from_string_literal.c`
- `test_invalid_51__omitted_array_size_requires_string_literal_initializer.c`

Snapshot tests:
- `test/print_ast/test_stage_14_06_char_array_string_init.c` +
  `.expected`
- `test/print_asm/test_stage_14_06_char_array_string_init_explicit.c`
  + `.expected`
- `test/print_asm/test_stage_14_06_char_array_string_init_inferred.c`
  + `.expected`

### Documentation
- `docs/grammar.md`: update `<declaration>` so the array-size
  integer literal is optional, with a note about the string-literal
  restriction.
- `README.md`: update the "Through stage" line, refresh the test
  count, and remove Stage 14-06 from "Not yet supported".

### Commit
Single commit at the end of the stage.
