# Stage 55-09 Kickoff: Bitwise and Shift Operators in Preprocessor

## Spec Summary

Stage 55-09 adds support for bitwise and shift operators to the preprocessor `#if`/`#elif` conditional expression evaluator. These operators enable more complex compile-time condition evaluation aligned with standard C operator precedence.

**Operators to support:**
- `~` (unary bitwise complement)
- `&` (bitwise AND)
- `^` (bitwise XOR)
- `|` (bitwise OR)
- `<<` (left shift)
- `>>` (right shift)

## Tokenizer Changes
**None.** Existing tokenizer already recognizes all six operators in preprocessor context.

## Parser Changes
**None.** Preprocessor conditional evaluation uses a custom expression parser (not the standard C expression parser).

## AST Changes
**None.** No new AST nodes needed.

## Code Generation Changes
**None.** Preprocessor evaluation is internal; no generated code changes.

## Preprocessor Changes

### Summary
Extend `eval_cond_*` family of functions in `src/preprocessor.c` to implement bitwise and shift operators with correct precedence. Insert three new functions into the precedence chain and update two existing functions.

### Implementation Order

1. **Add `~` to `eval_cond_unary`** — handles unary bitwise complement
2. **Add forward declarations** — for new functions
3. **Add `eval_cond_shift`** — evaluates `<<` and `>>` between additive and relational tiers
4. **Add `eval_cond_bitwise_and`** — evaluates `&` between equality and logical_and tiers
5. **Add `eval_cond_bitwise_xor`** — evaluates `^` between bitwise_and and bitwise_or tiers
6. **Add `eval_cond_bitwise_or`** — evaluates `|` between bitwise_xor and logical_and tiers
7. **Update `eval_cond_relational`** — call `eval_cond_shift` instead of `eval_cond_additive`
8. **Update `eval_cond_logical_and`** — call `eval_cond_bitwise_or` instead of `eval_cond_equality`

### Precedence Chain (Tightest → Loosest)

```
primary (parenthesized, defined(), literals, identifiers-as-0)
  ↑
unary (!, +, -, ~)
  ↑
multiplicative (*, /, %)
  ↑
additive (+, -)
  ↑
shift (<<, >>)
  ↑
relational (<, <=, >, >=)
  ↑
equality (==, !=)
  ↑
bitwise_and (&)
  ↑
bitwise_xor (^)
  ↑
bitwise_or (|)
  ↑
logical_and (&&)
  ↑
logical_or (||)
```

## Test Plan

Create 10 test files in `test/valid/` covering:

1. **Unary bitwise complement (`~`)** — operator returns two's complement; verify truthy/falsy semantics
2. **Bitwise AND (`&`)** — both operands true → true; one false → false
3. **Bitwise XOR (`^`)** — differing bits → true; same bits → false (two tests)
4. **Bitwise OR (`|`)** — at least one operand true → true; both false → false
5. **Left shift (`<<`)** — value shifted by amount
6. **Right shift (`>>`)** — value shifted by amount; verify zero-fill or sign-extend
7. **Right shift chaining (`>> >> >>`)** — multiple consecutive shifts (left-associative)
8. **Mixed precedence (`1 + 2 << 3`)** — addition before shift (`3 << 3 = 24`, not `1 + (2 << 3) = 9`)
9. **Bitwise AND with equality (`(6 & 3) == 2`)** — parentheses override precedence

## Known Ambiguities & Resolutions

### Ambiguity 1: `~VALUE` Test Specification

**Issue:** Spec defined `#define VALUE 1` with expected result `1` (falsy). However, mathematically `~1 = -2`, which is truthy in C conditionals. This is contradictory.

**Resolution:** Change test to `#define VALUE -1`. Then `~(-1) = 0` (falsy), so the condition fails and the `#else` branch (returning 1) is taken. Result matches spec expectation.

### Ambiguity 2: Missing `#endif`

**Issue:** Test case for `1 + 2 << 3` lacks closing `#endif` directive.

**Resolution:** Add `#endif` in test file for syntactic correctness.

## Implementation Notes

- All operations use `long` integer type for consistency with existing preprocessor evaluation.
- Shift operators: verify behavior with negative numbers and overflow (implementation-defined in C).
- No changes to tokenizer, parser, or AST; this is purely a preprocessor expression evaluation feature.
