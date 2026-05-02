# Stage-15-03 Kickoff

## Spec Summary

Stage 15-03 is a verification and integration stage for character
literals. The AST node `AST_CHAR_LITERAL`, parser primary-expression
acceptance of `TOKEN_CHAR_LITERAL`, and codegen for the literal were
all delivered in stage 15-02. This stage confirms — through new
integration tests — that the existing wiring lets a character literal
substitute anywhere an integer expression is allowed:

- `return 'A';`
- variable initialization (`char`, `int`, `long`)
- assignment
- arithmetic (`'A' + 1`)
- comparison (`'A' == 65`)
- truthy / falsy use in an `if` condition (`'A'` truthy, `'\0'` falsy)
- element assignment in a `char` array (`s[0] = 'H';`)

It also confirms that assignment to a smaller integer type uses the
existing conversion / truncation behavior (no new code path for
`char c = 'A';`) and that no special `char`-typed expression form was
introduced for the literal — character constants stay typed `int`,
matching C semantics.

## Spec Issues Called Out

1. The "NUL character in condition" test in the spec contains a typo:
   `int ('\0') {` should read `if ('\0') {`. The expected exit code
   (3) and the then/else branches confirm the intent. Implementation
   uses `if`.
2. The Requirements bullet refers to `AST_CHAR_INTERAL` — this is a
   typo for `AST_CHAR_LITERAL`. The node verified in the codebase is
   `AST_CHAR_LITERAL`.

## Verification of Existing Implementation (no changes required)

- `src/parser.c:131-140` — `TOKEN_CHAR_LITERAL` is consumed by
  `parse_primary_expression`, producing `AST_CHAR_LITERAL` with
  `decl_type = TYPE_INT` and the decoded byte at `value[0]`.
- `src/codegen.c:419-421` — `expr_result_type` returns `TYPE_INT`
  for `AST_CHAR_LITERAL`.
- `src/codegen.c:513-521` — `codegen_expression` emits
  `mov eax, <int>` for the decoded byte and records
  `result_type = TYPE_INT`.

No source-level changes (lexer, parser, AST, codegen) are needed. The
stage is purely additive: integration tests plus documentation refresh.

## Planned Changes

### Tokenizer
None.

### Parser
None.

### AST
None.

### Code Generator
None.

### Tests (new in `test/valid/`)
- `test_stage_15_03_char_literal_assign_char__65.c`
- `test_stage_15_03_char_literal_assign_int__65.c`
- `test_stage_15_03_char_literal_assign_long__65.c`
- `test_stage_15_03_char_literal_eq_int__1.c`
- `test_stage_15_03_char_literal_if_truthy__7.c`
- `test_stage_15_03_char_literal_if_nul_falsy__3.c`
- `test_stage_15_03_char_literal_array_element_assign__72.c`

The spec's `return 'A';` and `return 'A' + 1;` cases are already
covered by `test_char_literal_uppercase_A__65.c` and
`test_char_literal_arith__66.c` from stage 15-02 and are not
re-added.

### Documentation
- `docs/grammar.md` — no changes (the production
  `<primary_expression> ::= ... | <character_literal>` and the
  "character constant has type int" note were added in stage 15-02).
- `README.md` — bump the "Through stage 15-02" header to
  "Through stage 15-03"; update the character-literal bullet to note
  they are valid integer-context expressions; refresh aggregate test
  totals.

### Commit
A single stage-15-03 commit covering the new tests and the README
bump, after the full suite passes.

## Test Plan

After adding the 7 integration tests, run:

```
cmake --build build
./test/run_all_tests.sh
```

Expectation: `Aggregate: 435 passed, 0 failed, 435 total` (428 prior
+ 7 new valid tests; no regressions).
