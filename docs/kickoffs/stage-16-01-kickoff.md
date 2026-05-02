# Stage-16-01 Kickoff

## Spec Summary

Stage 16-01 adds the binary remainder operator `%` to the language.
The work is end-to-end across the pipeline:

1. **Tokenizer** — introduce `TOKEN_PERCENT` for the `%` character
   (no compound `%=` form in this stage).
2. **Grammar** — extend `<multiplicative_expression>` to admit `%`
   alongside `*` and `/`, with the same left-associativity.
3. **Parser/AST** — `%` parses as an `AST_BINARY_OP` whose
   `value` is `"%"`, mirroring `*` and `/`.
4. **Semantics** — operands must be integer types
   (`char`/`short`/`int`/`long`). Pointers and array values are
   invalid operands. Division-by-zero / remainder-by-zero is not
   checked at this stage.
5. **Codegen** — for 32-bit: `cdq; idiv ecx; mov eax, edx`. For
   64-bit: `cqo; idiv rcx; mov rax, rdx`. Preserve the existing
   left-then-right evaluation order; the right operand must live
   in a register/memory before `idiv` (the existing binary-op
   prologue already places left in `rcx` and right in `rax`,
   which after `xchg` for `/` already covers the layout the spec
   describes).
6. **Tests** — basic remainder, precedence with `*` / `/`,
   left-associativity, variable operands, use in conditionals
   and loops, `long` operands, plus invalid: bare `%`, missing
   rhs (`1%`), missing lhs (`%1`), pointer operand, array
   operand.

## Spec Issues Called Out

1. **Variables test missing closing brace.** The spec's
   variables test source is:

   ```c
   int main() {
      int a = 5;
      int b = 4;
      return a % b;   // expect 1
   ```

   The closing `}` is absent. We treat this as a transcription
   error in the spec and add the closing brace in the test file.

2. **`int main() { return 1%; }` and `return %; }` rejection
   wording.** The spec lists these as invalid but does not
   prescribe a diagnostic message. The existing parser's
   "expected expression" / "expected ')'" diagnostic from
   `parse_primary` / `parser_expect` will fire when the right
   operand or operator's surroundings are missing. We pick a
   filename fragment that matches the actual diagnostic the
   parser emits.

3. **Array-operand rejection path.** The spec says array values
   are invalid as `%` operands. Stage 13-03 already decays
   array names to pointers in value contexts, so `A % b` will
   reach codegen with a pointer operand. The existing
   `error: operator '%s' not supported on pointer operands`
   diagnostic handles both the explicit pointer case and the
   array-decay case in one path; we rely on it rather than
   adding an array-specific guard.

4. **Pre-`idiv` register placement.** The spec stresses that
   `idiv` cannot take an immediate. Our existing binary-op
   prologue already evaluates left into `rax`, pushes, evaluates
   right into `rax`, then pops left back into `rcx` — so the
   right operand always lives in a register at the point of
   `idiv`, no additional spill is required.

5. **`promote_kind` for char/short.** The current `*`/`/` paths
   already promote to `int` (32-bit `eax`/`ecx`) for char/short
   operands via `common_arith_kind`. `%` follows the same path
   without any extra logic.

## Planned Changes

### Tokenizer
- `include/token.h` — add `TOKEN_PERCENT`.
- `src/lexer.c` — emit `TOKEN_PERCENT` for the `%` character.
- `src/compiler.c` — add `TOKEN_PERCENT` case to
  `token_type_name` for `--print-tokens` output.

### Parser
- `src/parser.c` — extend the `parse_term` loop to also accept
  `TOKEN_PERCENT`; AST is the existing `AST_BINARY_OP` with
  `value = "%"`.

### AST
- No new node types or fields.

### Code Generator
- `src/codegen.c` — extend the arithmetic-op classifier in
  `expr_result_type` and the main `AST_BINARY_OP` emission path
  in `codegen_expression` to include `%`. Emit:
  - 32-bit: `xchg eax, ecx; cdq; idiv ecx; mov eax, edx`
  - 64-bit: `xchg rax, rcx; cqo; idiv rcx; mov rax, rdx`
  Pointer / array (decayed) operands continue to be rejected by
  the existing pointer-operator guard.

### Tests
Valid (`test/valid/`):
- `test_remainder_basic_zero__0.c` — `return 1 % 1;`
- `test_remainder_basic_one__1.c` — `return 5 % 4;`
- `test_remainder_basic_two__2.c` — `return 20 % 6;`
- `test_remainder_precedence_mul__1.c` — `return 1 + 2 % 2 * 6;`
- `test_remainder_precedence_div__2.c` — `return 1 + 5 % 3 / 2;`
- `test_remainder_left_assoc__2.c` — `return 2 % 4 % 2;`
- `test_remainder_variables__1.c` — `int a = 5; int b = 4;
  return a % b;`
- `test_remainder_in_if__1.c` — `if (a % b)` falls through to
  `else { return 1; }`.
- `test_remainder_in_while__5.c` — `while (a % b) { rtn++; b++; }`
  with `a = 7, b = 2`, expecting `rtn == 5`.
- `test_remainder_long__2.c` — `long a = 20L; long b = 6L;
  return a % b;`

Invalid (`test/invalid/`):
- `test_invalid_66__expected_expression.c` — `return %;`
- `test_invalid_67__expected_expression.c` — `return 1%;`
- `test_invalid_68__expected_expression.c` — `return %1;`
- `test_invalid_69__not_supported_on_pointer_operands.c` —
  `int *p; int b = 1; return p % b;`
- `test_invalid_70__not_supported_on_pointer_operands.c` —
  `int A[8]; int b = 1; return A % b;` (relies on array decay).

### Documentation
- `docs/grammar.md` — extend `<multiplicative_expression>` to
  include `%`.
- `README.md` — bump "Through stage 15-04" to
  "Through stage 16-01"; mention the remainder operator; refresh
  the aggregate test totals.

### Commit
Single `stage-16-01` commit covering tokenizer / parser / codegen
/ tests / docs after the full suite passes.

## Test Plan

```
cmake --build build
./test/run_all_tests.sh
```

Expectation: all prior tests still pass and the 10 new valid + 5
new invalid tests pass, with no regressions.
