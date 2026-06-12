# Stage 104 Kickoff

## Summary of the Spec

Stage 104 extends the two compile-time constant-expression evaluators (token-stream in
`src/parser.c` and AST-based in `src/codegen.c`) to support the complete C99 operator set:

- **Relational operators**: `<`, `<=`, `>`, `>=`
- **Equality operators**: `==`, `!=`
- **Logical AND**: `&&` (result normalized to 0 or 1)
- **Logical OR**: `||` (result normalized to 0 or 1)
- **Ternary/conditional operator**: `?:` (with proper right-associativity)

The stage also fixes a pre-existing precedence bug where `eval_const_additive` and
`eval_const_shift` have their call order inverted, causing expressions like `3 + 1 << 2`
to evaluate as `3 + (1 << 2) = 7` instead of the correct `(3 + 1) << 2 = 16`.

---

## Required Tokenizer Changes

**None.** All operators are already tokenized by the lexer.

---

## Required Parser Changes

The token-stream constant-expression evaluator in `src/parser.c` requires the following changes:

1. **Fix additive/shift precedence swap** (Task 1a):
   - `eval_const_additive` must call `eval_const_multiplicative` (not `eval_const_shift`)
   - `eval_const_shift` must call `eval_const_additive` (not `eval_const_multiplicative`)
   - No reordering of functions needed; `eval_const_additive` is already before `eval_const_shift`

2. **Insert `eval_const_relational` and `eval_const_equality`** (Task 1b–1c):
   - Add `eval_const_relational` between `eval_const_shift` and `eval_const_bitwise_and`
   - Add `eval_const_equality` immediately after `eval_const_relational`
   - Update `eval_const_bitwise_and` to call `eval_const_equality` instead of `eval_const_additive`

3. **Add logical operators and ternary** (Task 2):
   - Add `eval_const_logical_and` between `eval_const_bitwise_or` and `eval_const_expr`
   - Add `eval_const_logical_or` immediately after `eval_const_logical_and`
   - Add `eval_const_conditional` immediately after `eval_const_logical_or`
   - Update `eval_const_expr` to call `eval_const_conditional` (instead of `eval_const_bitwise_or`)
   - Update the grammar comment block at the top of `eval_const_primary` to document the full operator precedence

---

## Required AST Changes

**None.** The ternary/conditional operator is already represented as `AST_CONDITIONAL_EXPR`
with three children (condition, true branch, false branch) by `parse_conditional`.

---

## Required Code Generation Changes

In `src/codegen.c`, the `eval_const_init` function must be extended:

1. **Add relational, equality, and logical operators to the `AST_BINARY_OP` block** (Task 3a):
   - After the existing `|` operator case, add cases for `<`, `<=`, `>`, `>=`, `==`, `!=`, `&&`, `||`
   - For `&&` and `||`, normalize the result to 0 or 1

2. **Add `AST_CONDITIONAL_EXPR` case** (Task 3b):
   - Implement short-circuit evaluation: evaluate only the taken branch
   - Add this as a separate case **before** the final `compile_error` fallthrough

---

## Test Plan

### Valid tests

**Parser evaluator (enum, case label, file-scope initializer):**
- Relational in enum
- Equality in enum
- Logical AND in enum
- Logical OR in enum
- Ternary in enum
- Additive/shift precedence fix (file-scope)
- Relational in case label
- Ternary in file-scope initializer

**Codegen evaluator (block-scope static):**
- Relational in static initializer
- Equality in static initializer
- Logical AND in static initializer
- Logical OR in static initializer (with normalization)
- Ternary in static initializer

### Invalid tests

- Relational with non-constant operand (parser) — should reject with "is not an integer constant expression"
- Ternary with non-constant condition (static init) — should reject with "must be a constant expression"

---

## Implementation Order

1. `src/parser.c` — Task 1a (fix additive/shift swap)
2. `src/parser.c` — Task 1b–1c (add relational/equality; update bitwise_and)
3. `src/parser.c` — Task 2 (add logical operators and ternary; update eval_const_expr; update grammar comment)
4. `src/codegen.c` — Task 3a–3b (extend eval_const_init)
5. `src/version.c` — bump stage to `"01040000"`
6. Add valid and invalid test files
7. Update documentation (README.md, docs/grammar.md)
8. Run self-host test

---

## Ambiguities and Decisions

**None identified.** The spec is clear and unambiguous. All operator semantics match C99,
and both evaluators have well-defined precedence chains. The grammar comment documents
the complete chain after this stage.

---

## Known Issues

The additive/shift precedence bug affects the current compiler only when an expression
mixes `+`/`-` and `<<`/`>>` without parentheses in a constant context (enum, case label,
file-scope initializer, or static block-scope initializer). The compiler's own source
does not use this pattern, so self-hosting will not be affected by the fix.

