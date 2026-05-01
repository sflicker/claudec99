# Stage-14-08 Kickoff

## Spec
`docs/stages/ClaudeC99-spec-stage-14-08-additional-invalid-tests-for-char-array-support.md`

## Stage Goal
Round out the invalid-test coverage for the string-literal and
char-array-initialization features delivered in stages 14-03 through
14-07. The spec explicitly forbids tokenizer, parser, AST, and
codegen changes — only new test files are added. The existing
diagnostics must already be clear and stable enough for filename-based
expected-fragment matching.

## Delta from Stage 14-07-01
1. Four new files in `test/invalid/`:
   - `test_invalid_52__invalid_escape_sequence.c` — `"\q"` (single-char
     unsupported escape, complementing `test_invalid_47` which covers
     the hex form `\x41`).
   - `test_invalid_53__assigning_pointer_to_non_pointer.c` —
     `int x = "hello";` (string literal to integer variable).
   - `test_invalid_54__assigning_pointer_to_non_pointer.c` —
     `char c = "x";` (string literal to scalar char).
   - `test_invalid_55__string_initializer_only_supported_for_char_arrays.c`
     — `int s[3] = "hi";` (non-char *explicit-size* array,
     complementing `test_invalid_49` which covers the inferred form
     `int s[] = "hi";`).
2. `README.md`: bump invalid-test count and add a stage-14-08 row.

## Existing coverage (already in `test/invalid/`)
- `43` unterminated string literal
- `44` newline in string literal
- `46` incompatible pointer type in initializer (`int *p = "hello"`)
- `47` invalid escape sequence (`"\x41"`)
- `48` array too small for string literal initializer
- `49` string initializer only supported for char arrays (inferred size)
- `50` array size required unless initialized from string literal
- `51` omitted array size requires string literal initializer

## Ambiguities Flagged
- **Typo in spec.** Under *Reject unsupported escape sequences*, the
  second example is written `char * = "\x41";` — the identifier
  between `*` and `=` is missing. Read in context the intent is
  clearly `char *s = "\x41";`, which is already covered by
  `test_invalid_47`. No new test is needed for this example; only the
  `"\q"` case adds new coverage.
- **`char s[] = 123;` example missing `int main()` wrapper.** The
  spec block under *Reject omitted array size with non-string
  initializer* shows the body without a surrounding function.
  `test_invalid_51` already wraps the case in `int main() { ... }`,
  matching the convention used by every other test in the suite. No
  action needed.
- **Two diagnostic forms for "string literal to scalar".** The
  compiler emits `assigning pointer to non pointer` for both
  `int x = "hello";` and `char c = "x";`. The spec separates these
  into two bullets, so two test files are added — both happen to
  match the same expected error fragment but exercise different
  type combinations.

## Planned Changes

### Tokenizer (`src/lexer.c`)
- No changes (forbidden by spec).

### Parser (`src/parser.c`)
- No changes (forbidden by spec).

### AST
- No changes (forbidden by spec).

### Code Generator (`src/codegen.c`)
- No changes (forbidden by spec).

### Tests
- `test/invalid/test_invalid_52__invalid_escape_sequence.c`
- `test/invalid/test_invalid_53__assigning_pointer_to_non_pointer.c`
- `test/invalid/test_invalid_54__assigning_pointer_to_non_pointer.c`
- `test/invalid/test_invalid_55__string_initializer_only_supported_for_char_arrays.c`

### Documentation
- `README.md`: update invalid-test count (49 → 53) and add a
  stage-14-08 row to the stage table.
- `docs/grammar.md`: no changes.

### Commit
Single commit at the end of the stage.
