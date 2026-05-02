# Stage-16-05 Kickoff

## Spec Summary

Stage 16-05 adds the eight remaining compound assignment operators:
`*=`, `/=`, `%=`, `<<=`, `>>=`, `&=`, `^=`, `|=`. All eight desugar
to `a op= b` → `a = a op b`, reusing existing AST_BINARY_OP nodes and
the full code-generation paths already in place for `*`, `/`, `%`,
`<<`, `>>`, `&`, `^`, and `|`. No new AST node types or codegen paths
are required.

## What changes from stage 16-04

1. **Tokenizer** — eight new tokens: `TOKEN_STAR_ASSIGN` (`*=`),
   `TOKEN_SLASH_ASSIGN` (`/=`), `TOKEN_PERCENT_ASSIGN` (`%=`),
   `TOKEN_LEFT_SHIFT_ASSIGN` (`<<=`), `TOKEN_RIGHT_SHIFT_ASSIGN`
   (`>>=`), `TOKEN_AMP_ASSIGN` (`&=`), `TOKEN_CARET_ASSIGN` (`^=`),
   `TOKEN_PIPE_ASSIGN` (`|=`).
2. **Grammar** — `<assignment_operator>` gains the eight new forms.
3. **Parser** — `parse_expression` extends its identifier-fast-path to
   recognize the eight new tokens; each desugars to `a = a op b`
   using the same `AST_BINARY_OP` construction as `+=`/`-=`.
4. **AST** — no new node types.
5. **Codegen** — no changes; desugared trees use existing paths.
6. **Tests** — one valid test per operator, plus print-tokens tests
   for the new token names.

## Spec Issues Called Out

1. **`%=` missing from the grammar rule, `&=` duplicated** — The spec
   grammar lists `&=` twice and omits `%=`. The task section correctly
   lists `%=` as a new operator. Implementing `%=` and correcting the
   grammar rule to match the task section.
2. **Missing space before `"+="` in grammar** — minor formatting issue,
   no semantic impact.

## Planned Changes

### Tokenizer
- `include/token.h` — add 8 new token constants after `TOKEN_MINUS_ASSIGN`.
- `src/lexer.c` — extend `*`, `/`, `%`, `<<`, `>>`, `&`, `^`, `|`
  branches to emit the compound-assignment form when followed by `=`.
- `src/compiler.c` — extend `token_type_name` for the 8 new constants.

### Parser
- `src/parser.c` — extend the identifier-fast-path condition in
  `parse_expression` and add a `const char *bin_op` lookup for the
  8 new operators. The desugar block already handles the general
  `a op= b → a = a op b` shape.

### AST
- No changes.

### Code Generator
- No changes.

### Tests

Valid (`test/valid/`):
- `test_mul_assign__42.c` — `int a=21; a *= 2; return a;`
- `test_div_assign__10.c` — `int a=100; a /= 10; return a;`
- `test_rem_assign__2.c` — `int a=17; a %= 5; return a;`
- `test_shl_assign__8.c` — `int a=1; a <<= 3; return a;`
- `test_shr_assign__4.c` — `int a=32; a >>= 3; return a;`
- `test_and_assign__2.c` — `int a=6; a &= 3; return a;`
- `test_xor_assign__5.c` — `int a=6; a ^= 3; return a;`
- `test_or_assign__7.c` — `int a=4; a |= 3; return a;`

Print-tokens (`test/print_tokens/`):
- `test_token_star_assign.c` / `.expected`
- `test_token_slash_assign.c` / `.expected`
- `test_token_percent_assign.c` / `.expected`
- `test_token_left_shift_assign.c` / `.expected`
- `test_token_right_shift_assign.c` / `.expected`
- `test_token_amp_assign.c` / `.expected`
- `test_token_caret_assign.c` / `.expected`
- `test_token_pipe_assign.c` / `.expected`

### Documentation
- `docs/grammar.md` — update `<assignment_operator>` to include all
  10 operators, fixing the `&=` duplicate and adding `%=`.
- `README.md` — bump "Through stage 16-04" to "Through stage 16-05";
  add compound-assignment bullet; refresh test totals.

### Commit
Single `stage-16-05` commit. Trailer:
`Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>`.

## Test Plan

```
cmake --build build
./test/run_all_tests.sh
```

Expectation: prior 535 tests still pass; 8 new valid + 8 new
print-tokens = 16 new tests; total 551 with no regressions.
