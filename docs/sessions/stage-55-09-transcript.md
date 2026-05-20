# stage-55-09 Transcript: Bitwise and Shift Operators in Preprocessor

## Summary

Stage 55-09 extends the preprocessor `#if`/`#elif` conditional expression evaluator to support bitwise and shift operators: `~` (unary bitwise complement), `&` (bitwise AND), `^` (bitwise XOR), `|` (bitwise OR), `<<` (left shift), and `>>` (right shift). The implementation maintains C's standard operator precedence, inserting shift operators between additive and relational tiers, and bitwise operators with the ordering `&` > `^` > `|` positioned between equality and logical-AND. No changes were required to the tokenizer, parser, or AST; all work was confined to the preprocessor's internal expression evaluator in `src/preprocessor.c`.

## Changes Made

### Step 1: Preprocessor Expression Functions

- Added four forward declarations (`eval_cond_shift`, `eval_cond_bitwise_and`, `eval_cond_bitwise_xor`, `eval_cond_bitwise_or`) to organize the precedence chain.
- Extended `eval_cond_unary` to recognize `~` token alongside existing `!`, `-`, `+` unary operators; applies bitwise complement to the operand value.

### Step 2: Shift Operators

- Implemented `eval_cond_shift` function handling `<<` and `>>` operators with left-to-right associativity.
- Positioned shift operators between additive and relational tiers in precedence (lower precedence than arithmetic, higher than comparison).
- Updated `eval_cond_relational` to call `eval_cond_shift` instead of `eval_cond_additive` for the initial operand parse and all subsequent RHS sub-expressions.

### Step 3: Bitwise AND, XOR, and OR Operators

- Implemented `eval_cond_bitwise_and` handling single `&` token (guarded against `&&` interpretation).
- Implemented `eval_cond_bitwise_xor` handling `^` token with left-to-right associativity.
- Implemented `eval_cond_bitwise_or` handling single `|` token (guarded against `||` interpretation).
- All three ordered with precedence: `&` tightest, then `^`, then `|`; all positioned between equality and logical-AND in the precedence chain.

### Step 4: Logical Operator Chain Update

- Updated `eval_cond_logical_and` to call `eval_cond_bitwise_or` instead of `eval_cond_equality` for its initial operand parse.

### Step 5: Macro Integer Parsing

- Extended macro replacement integer parser in `eval_cond_primary` to accept an optional leading `-` sign, enabling macros like `#define VALUE -1` to be correctly evaluated in `#if` expressions.

### Step 6: Test Cases

Created 10 new tests in `test/valid/`:

| Test Name | Operators | Expression | Expected |
|-----------|-----------|------------|----------|
| `test_pp_if_bitwise_not__1.c` | `~` | `#define VALUE -1; ~VALUE = 0` | falsy → 1 |
| `test_pp_if_bitwise_and_true__42.c` | `&` | `A=1, B=1; A & B = 1` | truthy → 42 |
| `test_pp_if_bitwise_xor_true__42.c` | `^` | `A=1, B=0; A ^ B = 1` | truthy → 42 |
| `test_pp_if_bitwise_xor_false__1.c` | `^` | `A=1, B=1; A ^ B = 0` | falsy → 1 |
| `test_pp_if_bitwise_or_true__42.c` | `\|` | `A=1, B=0; A \| B = 1` | truthy → 42 |
| `test_pp_if_shift_left__42.c` | `<<` | `A=1; A << 1 = 2` | truthy → 42 |
| `test_pp_if_shift_right_zero__1.c` | `>>` | `A=1; A >> 1 = 0` | falsy → 1 |
| `test_pp_if_add_shift_precedence__42.c` | `+`, `<<` | `1 + 2 << 3 = 24` | truthy → 42 |
| `test_pp_if_shift_right_chain__42.c` | `>>` (chained) | `16 >> 1 >> 2 = 2` | truthy → 42 |
| `test_pp_if_bitwise_and_equality__42.c` | `&`, `==` | `(6 & 3) == 2 = 1` | truthy → 42 |

### Step 7: Documentation Updates

- Updated `docs/grammar.md` preprocessor expression grammar to replace the old `<pp-unary-op>` rule with `~` included, and restructured the precedence chain to insert `<pp-shift-expression>`, `<pp-bitwise-and-expression>`, `<pp-bitwise-xor-expression>`, and `<pp-bitwise-or-expression>` at the correct tiers.
- Updated `README.md` to change stage reference from 55-08 to 55-09; added bitwise and shift operators to the list of supported preprocessor `#if`/`#elif` operators; updated test totals from 994 to 1004.

## Final Results

Build: Successful — no compilation errors.

Tests: **All 1004 tests pass** (624 valid, 194 invalid, 27 integration, 39 print-AST, 99 print-tokens, 21 print-asm). +10 new valid tests, all passing. No regressions.

## Session Flow

1. Read stage spec and kickoff document to understand operator precedence and test cases.
2. Examined existing preprocessor expression evaluator in `src/preprocessor.c` to understand the current precedence chain.
3. Resolved spec ambiguity: clarified that `~VALUE` test needed `VALUE = -1` (not `VALUE = 1`) to produce falsy result.
4. Identified missing `#endif` in one spec test case and added it in the implementation.
5. Extended `eval_cond_unary` to recognize and apply `~` operator.
6. Implemented shift operator tier (`eval_cond_shift`) with left-associativity.
7. Implemented bitwise AND tier (`eval_cond_bitwise_and`) with single-`&` guarding.
8. Implemented bitwise XOR tier (`eval_cond_bitwise_xor`).
9. Implemented bitwise OR tier (`eval_cond_bitwise_or`) with single-`|` guarding.
10. Updated relational and logical-AND tiers to call new functions in the precedence chain.
11. Extended macro integer parser to accept optional leading `-` sign.
12. Created 10 test files covering all new operators and precedence interactions.
13. Ran full test suite: 1004 passed, 0 failed.
14. Updated `docs/grammar.md` preprocessor section with new grammar rules.
15. Updated `README.md` with stage 55-09 reference, revised preprocessor capabilities, and updated test totals.
16. Recorded milestone summary and this transcript.

