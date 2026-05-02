# Stage-16-02 Kickoff

## Spec Summary

Stage 16-02 adds the unary bitwise complement operator `~`. Operands
must be integer types (`char`, `short`, `int`, `long`); pointer and
array operands are invalid. Integer promotion applies to the operand
so `char`/`short`/`int` produce an `int` result via `not eax`, and
`long` produces a `long` result via `not rax`. `~` joins the existing
unary operator set at the same precedence as `+`, `-`, `!`, `++`,
`--`, `*`, `&`.

## What changes from stage 16-01

1. **Tokenizer** — introduce `TOKEN_TILDE` for the `~` character (no
   compound `~=` form in this stage).
2. **Grammar** — extend `<unary_operator>` to include `~`.
3. **Parser/AST** — `~` parses as `AST_UNARY_OP` with `value = "~"`,
   identical shape to `!` / unary `-`.
4. **Semantics** — operands must be integer types. Pointer and array
   (post-decay) operands are rejected at codegen with the existing
   "operator not supported on pointer operands" diagnostic.
5. **Codegen** — for promoted-int operands: `not eax`. For long
   operands: `not rax`. Char/short already sign-extend into `eax` on
   load, so no extra promotion code is needed.
6. **`!` tightening** — `!` is also tightened in this stage to reject
   pointer/array operands (per user direction; the stage spec itself
   only requires this for `~`).
7. **Tests** — basic complement, nested, variable operands at every
   integer width, precedence vs `+`, plus invalid pointer/array
   operand cases for both `~` and `!`.

## Spec Issues Called Out

1. **`!` rejection of pointer operands** is *not* in the stage 16-02
   spec, but the user has directed it be added alongside `~` for
   consistency. The diagnostic uses the same wording as `%` and `~`:
   `error: operator '!' not supported on pointer operands`. Recorded
   here as a deliberate scope addition.

2. **Exit codes 253 and 255**. The spec's expected exit codes
   (`-3` → 253, `-1` → 255) match the Linux convention `(unsigned
   char)signed_value`, which is how earlier stages encode negative
   `return` values in the test filename `__N` suffix.

3. **Out-of-scope**. Unsigned operands. The language has no unsigned
   support in any earlier stage, so this is a non-issue.

## Planned Changes

### Tokenizer
- `include/token.h` — add `TOKEN_TILDE` to the `TokenType` enum.
- `src/lexer.c` — emit `TOKEN_TILDE` for the `~` character.
- `src/compiler.c` — add `TOKEN_TILDE` case to `token_type_name`
  for `--print-tokens` output.

### Parser
- `src/parser.c` — extend `parse_unary` to also accept
  `TOKEN_TILDE`; AST is `AST_UNARY_OP` with `value = "~"`. Update
  the `<unary_expr>` grammar comment to list `~`.

### AST
- No new node types or fields.

### Code Generator
- `src/codegen.c`:
  - Extend the `AST_UNARY_OP` arm of `expr_result_type` so `~`
    yields `promote_kind(operand)` (mirrors `+`/`-`).
  - Extend the `AST_UNARY_OP` emission branch in
    `codegen_expression`:
    - New `~` case. Reject pointer operands with
      `error: operator '~' not supported on pointer operands`.
      For `TYPE_LONG` operand: `not rax`, result type `TYPE_LONG`.
      Otherwise: `not eax`, result type `TYPE_INT`.
    - Tighten existing `!` case to also reject pointer operands
      with `error: operator '!' not supported on pointer operands`.

### Tests

Valid (`test/valid/`):
- `test_complement_basic_neg3__253.c` — `return ~2;`
- `test_complement_basic_neg1__255.c` — `return ~0;`
- `test_complement_basic_zero__0.c` — `return ~-1;`
- `test_complement_nested__5.c` — `return ~~5;`
- `test_complement_var_int__253.c` — `int a = 2; return ~a;`
- `test_complement_var_char__253.c` — `char a = 2; return ~a;`
- `test_complement_var_short__253.c` — `short a = 2; return ~a;`
- `test_complement_var_long__253.c` —
  `long f() { long a = 2L; return ~a; } int main() { return f(); }`
- `test_complement_precedence__1.c` — `return ~2 + 4;`

Invalid (`test/invalid/`):
- `test_invalid_71__not_supported_on_pointer_operands.c` —
  `int a = 2; int *p = &a; return ~p;`
- `test_invalid_72__not_supported_on_pointer_operands.c` —
  `int A[2]; return ~A;` (relies on array decay).
- `test_invalid_73__not_supported_on_pointer_operands.c` —
  `int a = 2; int *p = &a; return !p;` (`!` tightening).
- `test_invalid_74__not_supported_on_pointer_operands.c` —
  `int A[2]; return !A;` (`!` tightening, array decay).

Print-tokens (`test/print_tokens/`):
- `test_token_tilde.c` / `.expected` — a bare `~` lexes as
  `TOKEN_TILDE`.

Print-asm (`test/print_asm/`):
- `test_stage_16_02_complement_int.c` / `.expected` — `return ~2;`
  locks in `not eax`.
- `test_stage_16_02_complement_long.c` / `.expected` — long-typed
  variable case locks in `not rax`.

### Documentation
- `docs/grammar.md` — `<unary_operator>` extended to include `~`.
- `README.md` — bump "Through stage 16-01" to "Through stage 16-02";
  mention bitwise complement under unary operators; refresh test
  totals.

### Commit
Single `stage-16-02` commit covering tokenizer / parser / codegen /
tests / docs after the full suite passes.

## Test Plan

```
cmake --build build
./test/run_all_tests.sh
```

Expectation: all prior tests still pass, the 9 new valid + 4 new
invalid + 1 new print-tokens + 2 new print-asm tests pass, and there
are no regressions.
