# stage-104 Transcript: Complete Constant-Expression Evaluator

## Summary

Stage 104 extends both compile-time constant-expression evaluators to support the full C99 integer constant expression operator set. Two independent evaluators exist in the compiler: eval_const_expr in src/parser.c (token-stream based, used for case labels, enum values, array designators, and file-scope initializers) and eval_const_init in src/codegen.c (AST-based, used for block-scope static scalar initializers). Prior to this stage, both evaluators supported only arithmetic, bitwise, and shift operators. Stage 104 adds relational operators (<, <=, >, >=), equality operators (==, !=), logical AND (&&), logical OR (||), and the ternary conditional operator (?:), completing the operator set to match C99 requirements.

A pre-existing precedence bug is also fixed: eval_const_additive and eval_const_shift in the parser evaluator had inverted call order, causing expressions like `3 + 1 << 2` to evaluate incorrectly as 7 instead of the correct 16.

## Changes Made

### Step 1: Parser — Fix Additive/Shift Precedence

- Swapped call order in eval_const_additive and eval_const_shift so that eval_const_additive now calls eval_const_multiplicative and eval_const_shift now calls eval_const_additive.
- This corrects the operator precedence: shift (<<, >>) now has lower precedence than additive (+, -), matching C99 standards.
- Fixes silent evaluation bug where `3 + 1 << 2` was producing 7 instead of 16.

### Step 2: Parser — Add Relational and Equality Operators

- Added eval_const_relational function to handle <, <=, >, >= operators.
- Added eval_const_equality function to handle == and != operators.
- Updated eval_const_bitwise_and to call eval_const_equality (was calling eval_const_additive directly before the fix in Step 1).
- Functions follow standard recursive-descent pattern with left-associative operators.

### Step 3: Parser — Add Logical and Ternary Operators

- Added eval_const_logical_and function to handle && operator with result normalization to 0 or 1.
- Added eval_const_logical_or function to handle || operator with result normalization to 0 or 1.
- Added eval_const_conditional function to handle ?: ternary operator with right-associativity matching C99 semantics.
- Updated eval_const_expr to call eval_const_conditional (was calling eval_const_bitwise_or).
- Updated grammar comment block in eval_const_primary to document the complete 12-level grammar.

### Step 4: Codegen — Extend eval_const_init

- Added six relational and equality operator branches to AST_BINARY_OP block: <, <=, >, >=, ==, !=.
- Added two logical operator branches to AST_BINARY_OP block: &&, || (with result normalization to 0 or 1).
- Added AST_CONDITIONAL_EXPR case with lazy evaluation: only evaluates the selected branch (condition ? true_branch : false_branch), preventing errors from unused branches.

### Step 5: Version

- Bumped VERSION_STAGE in src/version.c to "01040000".

### Step 6: Tests

- Added 13 valid tests covering all new operators in both parser evaluator contexts (enum, case, file-scope) and codegen evaluator context (block-scope static scalars).
- Added 2 invalid tests detecting non-constant operands in relational/ternary contexts.
- Precedence fix test (test_file_scope_const_expr_additive_shift_prec__0.c) verifies 3+1<<2 evaluates to 16.

## Final Results

All 1584 tests pass (909 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). No regressions. Self-host C0→C1→C2 cycle completes successfully with identical behavior across all three compiler generations (versions 00.02.01040000.00837, 00838, 00839).

## Session Flow

1. Read stage 104 specification detailing the two constant-expression evaluators and the precedence bug.
2. Reviewed existing eval_const_expr and eval_const_init implementations.
3. Verified the additive/shift precedence bug by tracing through example expressions.
4. Implemented Step 1 (precedence fix) in parser.c.
5. Implemented Step 2 (relational/equality operators) in parser.c.
6. Implemented Step 3 (logical/ternary operators) in parser.c.
7. Implemented Step 4 (extend eval_const_init) in codegen.c.
8. Bumped version number in src/version.c.
9. Compiled cleanly and ran full test suite.
10. Ran self-host C0→C1→C2 cycle; all three generations compiled the compiler correctly and produced identical binaries.
