# Stage-15-04 Kickoff

## Spec Summary

Stage 15-04 completes the character-literal feature begun in stages
15-01 / 15-02 / 15-03. Earlier stages added char-literal tokenization,
the `AST_CHAR_LITERAL` node, codegen, and integration with the integer
type system; 7 of the 12 simple-escape sequences (`\n`, `\t`, `\r`,
`\\`, `\'`, `\"`, `\0`) were already supported. This stage:

1. Fills in the remaining simple escapes:
   `\a` (7), `\b` (8), `\f` (12), `\v` (11), `\?` (63).
2. Verifies the existing invalid-literal diagnostics
   (empty literal, multi-character constant, unknown escape,
   unterminated literal) and adds explicit handling for a raw
   newline inside a character literal.
3. Adds the missing valid and invalid test coverage and refreshes
   documentation, including new through-stage-15-04 status snapshots.

Octal `\NNN` (other than the special `\0`), hex `\xNN`, multi-character
constants, and wide-character literals (`L'A'`, `u'A'`, `U'A'`) remain
out of scope — they are still rejected by the lexer.

## Spec Issues Called Out

1. **Suggested vs existing diagnostic wording.** The spec suggests
   `unknown escape sequence in character literal` but the project
   currently emits `invalid escape sequence in character literal`.
   The spec explicitly allows the existing wording (“Exact wording
   does not need to match the strings if the existing diagnostics are
   already clear”), so the message is left as-is and the test fragment
   matches the existing diagnostic.
2. **`'\` vs `'\\'` test source.** Section 2 lists `'\` (no closing
   quote) as the unterminated-escape-sequence example, but the
   "Add invalid tests for" list shows `int main() { return '\'; }`,
   which the lexer parses as `\'` (escaped single quote) followed by
   no closing quote — i.e. an unterminated literal. We follow the
   actual test source from the spec: the test is named for the
   `unterminated character literal` diagnostic the lexer produces.
3. **Newline-in-character-literal diagnostic.** The spec calls this
   out as a distinct case from the generic unterminated literal, with
   suggested message `newline in character literal`. The lexer
   currently lumps `\n` and `\0` together as "unterminated character
   literal". The string-literal lexer already keeps these two cases
   separate (`unterminated string literal` vs
   `newline in string literal`), so we mirror that split for char
   literals — small, mechanical change that brings the diagnostic in
   line with both the spec suggestion and existing project style.
4. **Status filename hyphenation.** The spec uses
   `project-features-through-stage-15-04.md` (correct spelling),
   while the historical file in `status/` is
   `project-features-throgh-stage-12-04.md` ("throgh"). We use the
   spelling from the new spec for the new files.

## Planned Changes

### Tokenizer (`src/lexer.c`)
- Add cases `'a'` (7), `'b'` (8), `'f'` (12), `'v'` (11), `'?'` (63)
  to the character-literal escape switch. The string-literal escape
  switch is left unchanged — Section 1 of the spec is character-only.
- Split the existing `ch == '\0' || ch == '\n'` early-exit branch so
  a raw newline produces `error: newline in character literal` and
  EOF continues to produce `error: unterminated character literal`.

### Parser
None.

### AST
None.

### Code Generator
None.

### Tests

New valid tests (`test/valid/`):
- `test_char_literal_escape_a__7.c`
- `test_char_literal_escape_b__8.c`
- `test_char_literal_escape_f__12.c`
- `test_char_literal_escape_v__11.c`
- `test_char_literal_escape_question__63.c`

The other 7 simple escapes (`\n`, `\t`, `\r`, `\\`, `\'`, `\"`, `\0`)
already have valid tests in `test/valid/test_char_literal_escape_*`
and are not duplicated.

New invalid tests (`test/invalid/`):
- `test_invalid_63__invalid_escape_sequence.c` — `'\q'`
- `test_invalid_64__unterminated_character_literal.c` — `'\'` (the
  spec's "unterminated escape" source; lexer reports it as an
  unterminated literal)
- `test_invalid_65__newline_in_character_literal.c` — raw newline
  inside a character literal

The empty-literal and multi-character-constant cases are already
covered by `test_invalid_57__empty_character_literal.c` and
`test_invalid_58__multi_character_constant.c` (stage 15-02) and are
not duplicated.

### Documentation
- `docs/grammar.md` — extend `<character_escape_sequence>` to list
  the additional simple escapes.
- `README.md` — bump the "Through stage 15-03" header to
  "Through stage 15-04"; update the character-literal bullet to
  enumerate the full simple-escape set; refresh aggregate test totals.
- `status/project-features-through-stage-15-04.md` (new) — features
  snapshot, modeled on the existing `project-features-*` file.
- `status/project-status-through-stage-15-04.md` (new) — full status
  snapshot, modeled on the existing
  `project-status-through-stage-12-04.md`.

### Commit
Single `stage-15-04` commit covering the lexer change, new tests, and
documentation updates after the full suite passes.

## Test Plan

After the lexer change and new tests are in place:

```
cmake --build build
./test/run_all_tests.sh
```

Expectation: all prior tests still pass and the 5 new valid + 3 new
invalid tests pass, with no other regressions.
