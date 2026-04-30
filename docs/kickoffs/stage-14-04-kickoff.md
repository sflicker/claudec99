# Stage-14-04 Kickoff

## Spec
`docs/stages/ClaudeC99-spec-stage-14-04-string-literals-as-char-star-expressions.md`

## Stage Goal
Promote a string literal expression so it produces a value of type
`char *` (via array-to-pointer decay), enabling:

- `char *s = "hi";` initialization
- `s = "hi";` assignment
- Indexing through the decayed pointer (`s[i]`)
- Pointer arithmetic on the decayed pointer (`*(s + 1)`)
- Rejection of incompatible pointer initializers like `int *p = "hello";`

## Delta from Stage 14-03
Stage 14-03 emitted each string literal into a `.rodata` section and
produced `lea rax, [rel Lstr<N>]` when used as a value, but typed the
result as `TYPE_LONG` (opaque address). Stage 14-04 changes that result
type to `TYPE_POINTER` with `full_type = pointer-to-char`, so the
existing pointer-init, pointer-assign, pointer-arith, and
pointer-subscript paths fire automatically.

## Ambiguities Flagged
- "Existing tests must continue to pass" conflicts with the Stage
  14-03 print_asm tests (`int main() { return "hi"; }`). Once string
  literals decay to `char *`, returning one from `int main()` trips
  the existing strict pointer/non-pointer return-type check from
  Stage 12-05. Those snapshot tests must be refreshed (source +
  `.expected`) to use a `char *s = "..."` form. No real regression
  in valid/invalid/print_ast/print_tokens.
- The spec's invalid test is missing its closing `}`. Treated as a
  typo and added in the test file.

## Planned Changes

### Tokenizer
- No changes.

### Parser
- No changes. `parse_primary` already builds `AST_STRING_LITERAL`
  with `full_type = char[N+1]`.

### AST
- No changes.

### Code Generation (`src/codegen.c`)
- `AST_STRING_LITERAL` branch in `codegen_expression`: after pool
  registration and `lea`, set
  `result_type = TYPE_POINTER` and
  `full_type = type_pointer(type_char())` so the literal decays to
  `char *`.
- `codegen_emit_string_pool`: derive `byte_len` from
  `strlen(s->value)` instead of `s->full_type->length - 1`, since
  the AST node's `full_type` is now the decayed pointer.

### Tests
Valid (`test/valid/`):
- `test_string_literal_init_index_0__104.c`
- `test_string_literal_init_index_1__105.c`
- `test_string_literal_assign_then_index__105.c`
- `test_string_literal_empty__0.c`
- `test_string_literal_init_index_2__99.c`
- `test_string_literal_pointer_arith_deref__98.c`

Invalid (`test/invalid/`):
- `test_invalid_46__incompatible_pointer_type_in_initializer.c`

Refreshed snapshots (`test/print_asm/`):
- `test_stage_14_03_string_literal_basic.c` and `.expected`
- `test_stage_14_03_string_literal_empty.c` and `.expected`
- `test_stage_14_03_string_literal_one.c` and `.expected`

### Documentation
- No grammar changes (the grammar already lists `<string_literal>`
  as a primary expression). Save milestone + transcript at the end.

### Commit
Single commit at the end of the stage.
